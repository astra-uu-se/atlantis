#include "atlantis/fznBackend.hpp"

#include <fznparser/parser.hpp>
#include <thread>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchController.hpp"
#include "atlantis/search/threadController.hpp"
#include "atlantis/solverThread.hpp"
#include "atlantis/utils/fznOutput.hpp"

namespace atlantis {

void FznBackend::displaySolution(
    const search::SavedAssignment& assignment) const {
  _fznOutput->displaySolution(std::cout, assignment.getOutputValues());

  std::cout << "----------\n";
}

void FznBackend::onSolutionDefault(const search::SavedAssignment& assignment,
                                   search::ThreadController& controller,
                                   Int threadId) const {
  const bool savedSolution = controller.trySolution(threadId, assignment);

  if (savedSolution && controller.shouldPrint(threadId)) {
    displaySolution(assignment);
    controller.hasPrinted();
  }
}

void FznBackend::onFinishDefault(bool hadSol) {
  if (!hadSol) {
    std::cout << "=====UNKNOWN=====\n";
  }
}

FznBackend::FznBackend(fznparser::Model&& model,
                       const std::uint_fast32_t threadCount,
                       search::SearchType searchType)
    : _invariantGraph(
          std::make_shared<invariantgraph::FznInvariantGraph>(true)),
      _model(std::make_shared<fznparser::Model>(std::move(model))),
      _seed(std::time(nullptr)),
      _threadCount(threadCount),
      _searchType(searchType),
      _onSolution([&](const search::SavedAssignment& assignment,
                      search::ThreadController& controller, Int threadId) {
        onSolutionDefault(assignment, controller, threadId);
      }) {}

FznBackend::FznBackend(logging::Logger& logger,
                       std::filesystem::path&& modelFile,
                       const uint_fast32_t threadCount,
                       search::SearchType searchType)
    : FznBackend(logger.timedFunction<fznparser::Model>(
                     "parsing FlatZinc",
                     [&] {
                       auto m = fznparser::parseFznFile(modelFile);
                       logger.debug("Found {:d} variable(s)", m.vars().size());
                       logger.debug("Found {:d} constraint(s)",
                                    m.constraints().size());
                       return m;
                     }),
                 threadCount, searchType) {}

void FznBackend::solve(logging::Logger& logger) {
  // Shared data
  fznparser::ProblemType problemType = _model->solveType().problemType();
  auto schedule = _annealingScheduleFactory.create();

  auto controller = std::make_shared<search::ThreadController>();

  std::vector<std::thread> threads;
  std::cerr << "Thread count is " << _threadCount << "\n";

  _invariantGraph->open();
  logger.timedProcedure("building invariant graph",
                        [&] { _invariantGraph->build(*_model); });
  _invariantGraph->close();
  _fznOutput =
      std::make_unique<FznOutput>(_invariantGraph->generateFznOutput());

  for (size_t threadId = 0; threadId < _threadCount; threadId++) {
    threads.emplace_back([&logger, &problemType, &schedule, threadId,
                          &controller, this] {
      auto thread =
          SolverThread(_invariantGraph, _fznOutput->varNodeIds(), problemType,
                       schedule, threadId, controller, _searchType,
                       _seed + threadId, _timelimit, _onSolution, _onFinish);
      thread.solve(logger);
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  if (controller->getBestThreadId() >= 0) {
    std::cerr << "Best result is " << controller->getCost().toString()
              << " from thread " << controller->getBestThreadId() << std::endl;
  } else {
    std::cerr << "No solution found!" << std::endl;
  }
}

}  // namespace atlantis

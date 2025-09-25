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
    const search::SavedAssignment& assignment, const FznOutput& output) {
  output.displaySolution(std::cout, assignment.getOutputValues());

  std::cout << "----------\n";
}

search::SavedAssignment FznBackend::onSolutionDefault(
    const search::Assignment& assignment,
    const FznOutput& output,
    search::ThreadController& controller,
    Int threadId) {
  auto savedAssignment = search::SavedAssignment(assignment, output.getSolverIds());
  savedAssignment = controller.trySolution(threadId, savedAssignment);

  if (savedAssignment.getCost().getObjective() ==
          assignment.getCost().getObjective() &&
      controller.shouldPrint(threadId)) {
    displaySolution(savedAssignment, output);
    controller.hasPrinted();
  }

  return savedAssignment;
}

void FznBackend::onFinishDefault(bool hadSol) {
  if (!hadSol) {
    std::cout << "=====UNKNOWN=====\n";
  }
}

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

  for (size_t threadId = 0; threadId < _threadCount; threadId++) {
    threads.emplace_back([&logger, &problemType, &schedule,
                          threadId, &controller, this] {
      auto thread =
          SolverThread(_invariantGraph, problemType, schedule, threadId,
                       controller,  _seed + threadId,
                       _timelimit, _onSolution, _onFinish);
      _dotFilePath.reset();  // InvariantGraph will only be saved once
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

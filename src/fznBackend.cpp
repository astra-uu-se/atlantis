#include "atlantis/fznBackend.hpp"

#include <fznparser/parser.hpp>
#include <thread>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/annealer.hpp"
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

void FznBackend::onSolutionDefault(
    [[maybe_unused]] const search::SavedAssignment& assignment,
    [[maybe_unused]] const search::ThreadController& controller,
    [[maybe_unused]] const Int threadId) const {
  // This currently does nothing.
}

void FznBackend::onFinishDefault([[maybe_unused]] bool hadSol) {
  // This currently does nothing.
}

void FznBackend::handleSolverIO(
    const std::shared_ptr<search::ThreadController>& threadController) const {
  size_t mostRecentSolution = 0;

  while (threadController->getNumFinishedThreads() < _threadCount) {
    threadController->awaitChanges();

    auto result = threadController->getNewerSolution(mostRecentSolution);
    if (!result.has_value()) continue;

    mostRecentSolution = result.value().first;
    displaySolution(result.value().second);
    threadController->solutionPrinted();
  }

  // Ensure the final solution is printed
  // When this runs all search threads have terminated.
  if (mostRecentSolution < threadController->getSolutionNumber()) {
    std::cout << "printing final solution! (previously printed "
              << mostRecentSolution << ", final is "
              << threadController->getSolutionNumber() << ")." << std::endl;
    search::SavedAssignment assignment = threadController->getSolution();
    displaySolution(assignment);
  }

  if (!threadController->hasSolution()) {
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
  _threadController = std::make_shared<search::ThreadController>(_threadCount);

  // TODO: refactor everywhere to use the shared pointer
  assert(_threads.empty());
  _threads.reserve(_threadCount);
  logger.info("Thread count is {}", _threadCount);

  _invariantGraph->open();
  logger.timedProcedure("building invariant graph",
                        [&] { _invariantGraph->build(*_model); });
  _invariantGraph->close();
  _fznOutput =
      std::make_unique<FznOutput>(_invariantGraph->generateFznOutput());

  for (size_t threadId = 0; threadId < _threadCount; threadId++) {
    _threads.emplace_back([&logger, &problemType, threadId, this] {
      auto thread =
          SolverThread(_invariantGraph, _fznOutput->varNodeIds(), problemType,
                       _annealingScheduleFactory.create(), threadId,
                       _threadController, _searchType, _seed + threadId,
                       _timelimit, _shouldStop, _onSolution, _onFinish);
      thread.solve(logger);
    });
  }
  handleSolverIO(_threadController);
}

void FznBackend::join(logging::Logger& logger) {
  if (_threads.empty()) {
    return;
  }
  for (auto& thread : _threads) {
    thread.join();
  }

  if (_threadController->getBestThreadId() >= 0) {
    logger.info("Best result is {} from thread {}",
                _threadController->getCost().toString(),
                _threadController->getBestThreadId());
  } else {
    logger.info("No solution found!");
  }
}

}  // namespace atlantis

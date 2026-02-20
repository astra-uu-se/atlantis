#include "atlantis/fznBackend.hpp"

#include <fznparser/parser.hpp>
#include <thread>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/annealer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/threadController.hpp"
#include "atlantis/solverThread.hpp"
#include "atlantis/utils/fznOutput.hpp"

namespace atlantis {

void FznBackend::onSolutionDefault(
    const search::SavedAssignment& assignment,
    const std::optional<
        std::vector<std::shared_ptr<search::SearchStatistics>>>&) const {
  _fznOutput->displaySolution(std::cout, assignment.getOutputValues());
  std::cout << "----------" << std::endl;
}

void FznBackend::onFinishDefault(const bool hasSatisfyingSolution) {
  if (!hasSatisfyingSolution) {
    std::cout << "=====UNKNOWN=====\n";
  }
}

void FznBackend::handleSolverNotifications(
    const std::shared_ptr<search::ThreadController>& threadController) const {
  size_t solutionId = 0;

  while (threadController->numFinishedThreads() < _threadCount) {
    threadController->awaitChanges();

    auto result = threadController->loadSolution(solutionId);
    if (!result.has_value()) {
      continue;
    }

    solutionId = result.value().first;
    _onSolution(result.value().second, threadController->getStats());
  }

  // Ensure the final solution is printed
  // When this runs all search threads have terminated.
  if (solutionId < threadController->solutionId()) {
    std::cout << "printing final solution! (previously printed " << solutionId
              << ", final is " << threadController->solutionId() << ")."
              << std::endl;
    _onSolution(threadController->solution(), threadController->getStats());
  }

  _onFinish(threadController->hasSolution() &&
            threadController->hasNoViolations());
}

FznBackend::FznBackend(fznparser::Model&& model,
                       const std::uint_fast32_t threadCount,
                       search::SearchType searchType)
    : _invariantGraph(
          std::make_shared<invariantgraph::FznInvariantGraph>(true)),
      _model(std::make_shared<fznparser::Model>(std::move(model))),
      _annealingScheduleFactory(
          std::make_shared<search::AnnealingScheduleFactory>()),
      _seed(std::time(nullptr)),
      _threadCount(threadCount),
      _searchType(searchType),
      _onSolution([&](const search::SavedAssignment& assignment,
                      const std::optional<std::vector<
                          std::shared_ptr<search::SearchStatistics>>>& stats) {
        onSolutionDefault(assignment, stats);
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
  auto selector = std::make_unique<search::ArmSelector>(_annealingScheduleFactory);
  _threadController = std::make_shared<search::ThreadController>(_threadCount, std::move(selector));

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
    _threads.emplace_back([this, threadId] {
      auto thread = SolverThread(*this, threadId);
      thread.solve();
    });
  }
  handleSolverNotifications(_threadController);
}

void FznBackend::join(logging::Logger& logger) {
  if (_threads.empty()) {
    return;
  }
  for (auto& thread : _threads) {
    thread.join();
  }

  if (_threadController->bestThreadId() >= 0) {
    logger.info("Best result is {} from thread {}",
                _threadController->cost().toString(),
                _threadController->bestThreadId());
  } else {
    logger.info("No solution found!");
  }
}

}  // namespace atlantis

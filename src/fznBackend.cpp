#include "atlantis/fznBackend.hpp"

#include <fznparser/parser.hpp>
#include <thread>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/annealer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/bandits/ExploreThenCommit.hpp"
#include "atlantis/search/bandits/KullbackLeiblerUpperConfidenceBound.hpp"
#include "atlantis/search/bandits/OCUCBn.hpp"
#include "atlantis/search/bandits/ThompsonSampling.hpp"
#include "atlantis/search/bandits/upperConfidenceBound.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchController.hpp"
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

void FznBackend::onFinishDefault(const SolveOutcome outcome) {
  switch (outcome) {
    case SolveOutcome::SATISFIABLE:
      return;
    case SolveOutcome::UNSATISFIABLE:
      std::cout << "=====UNSATISFIABLE=====\n";
      return;
    case SolveOutcome::UNKNOWN:
      std::cout << "=====UNKNOWN=====\n";
  }
}

void FznBackend::handleSolverNotifications(
    const std::shared_ptr<search::ThreadController>& threadController) const {
  size_t solutionId = 0;

  while (threadController->numFinishedThreads() < _threadCount) {
    threadController->awaitChanges();
    if (threadController->hasFatalError()) {
      threadController->rethrowFatalErrorIfAny();
    }

#ifndef MORE_STATS
    if (!threadController->hasNoViolations()) continue;
#endif

    auto result = threadController->loadSolution(solutionId);

    if (!result.has_value()) continue;

    solutionId = result.value().first;
    _onSolution(result.value().second, threadController->getStats());
  }

  threadController->rethrowFatalErrorIfAny();

  // Ensure the final solution is printed
  // When this runs all search threads have terminated.
  if (solutionId < threadController->solutionId() &&
      threadController->hasNoViolations()) {
    std::cout << "printing final solution! (previously printed " << solutionId
              << ", final is " << threadController->solutionId() << ")."
              << std::endl;
    _onSolution(threadController->solution(), threadController->getStats());
  }

  _onFinish(threadController->hasSolution() &&
                    threadController->hasNoViolations()
                ? SolveOutcome::SATISFIABLE
                : SolveOutcome::UNKNOWN);
}

FznBackend::FznBackend(fznparser::Model&& model,
                       const std::uint_fast32_t threadCount,
                       const search::SearchType searchType,
                       const search::BanditAlgorithm banditAlgorithm)
    : _invariantGraph(
          std::make_shared<invariantgraph::FznInvariantGraph>(true)),
      _model(std::make_shared<fznparser::Model>(std::move(model))),
      _annealingScheduleFactory(
          std::make_shared<search::AnnealingScheduleFactory>()),
      _seed(std::time(nullptr)),
      _threadCount(threadCount),
      _searchType(searchType),
      _banditAlgorithm(banditAlgorithm),
      _onSolution([&](const search::SavedAssignment& assignment,
                      const std::optional<std::vector<
                          std::shared_ptr<search::SearchStatistics>>>& stats) {
        onSolutionDefault(assignment, stats);
      }) {}

FznBackend::FznBackend(logging::Logger& logger,
                       std::filesystem::path&& modelFile,
                       const uint_fast32_t threadCount,
                       const search::SearchType searchType,
                       const search::BanditAlgorithm banditAlgorithm)
    : FznBackend(logger.timedFunction<fznparser::Model>(
                     "parsing FlatZinc",
                     [&] {
                       auto m = fznparser::parseFznFile(modelFile);
                       logger.debug("Found {:d} variable(s)", m.vars().size());
                       logger.debug("Found {:d} constraint(s)",
                                    m.constraints().size());
                       return m;
                     }),
                 threadCount, searchType, banditAlgorithm) {}

void FznBackend::solve(logging::Logger& logger) {
  // Shared data

  std::unique_ptr<search::ArmSelector> selector;
  switch (_banditAlgorithm) {
    case search::BanditAlgorithm::UCB:
      // printf("Using bandit algorithm upper confidence bound (UCB).\n");
      selector = std::make_unique<search::UpperConfidenceBound>(
          _annealingScheduleFactory, _onArmRecording);
      break;
    case search::BanditAlgorithm::KLUCB:
      // printf("Using bandit algorithm Kullback-Leibler upper confidence bound "
      //     "(KL-UCB).\n");
      selector = std::make_unique<search::KullbackLeiblerUpperConfidenceBound>(
          _annealingScheduleFactory, _onArmRecording);
      break;
    case search::BanditAlgorithm::OCUCBn:
      // printf("Using bandit algorithm anytime optimally confident upper "
      //        "confidence bound (OCUCB-n).\n");
      selector = std::make_unique<search::OCUCBn>(
          _annealingScheduleFactory, _onArmRecording);
      break;
    case search::BanditAlgorithm::Thompson:
      // printf("Using bandit algorithm Thompson sampling.\n");
      selector =
          std::make_unique<search::ThompsonSampling>(
            _annealingScheduleFactory, _onArmRecording);
      break;
    default:
      // printf("Using bandit algorithm explore-then-commit.\n");
      selector = std::make_unique<search::ExploreThenCommit>(
          _annealingScheduleFactory, _onArmRecording);
  }
  _threadController = std::make_shared<search::ThreadController>(
      _threadCount, std::move(selector));

  // TODO: refactor everywhere to use the shared pointer
  assert(_threads.empty());
  _threads.reserve(_threadCount);
  logger.info("Thread count is {}", _threadCount);

  _invariantGraph->open();
  try {
    logger.timedProcedure("building invariant graph",
                          [&] { _invariantGraph->build(*_model); });
    _invariantGraph->close();
  } catch (const InconsistencyException& e) {
    logger.warn("Invariant graph construction detected infeasibility: {}",
                e.what());
    _onFinish(SolveOutcome::UNSATISFIABLE);
    return;
  }
  _fznOutput =
      std::make_unique<FznOutput>(_invariantGraph->generateFznOutput());

  for (size_t threadId = 0; threadId < _threadCount; threadId++) {
    _threads.emplace_back([this, threadId] {
      auto thread = SolverThread(*this, threadId);
      thread.solve();
    });
  }
  handleSolverNotifications(_threadController);

  _threadController->showArmStats();
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

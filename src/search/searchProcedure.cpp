#include "atlantis/search/searchProcedure.hpp"

#include <chrono>
#include <iostream>

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/types.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/metaheuristic.hpp"
#include "atlantis/search/searchController.hpp"

namespace atlantis::search {

// TODO: either use or remove this function
SearchStatistics makeStats(const Statistic& rounds,
                           const Statistic& initialisations,
                           const Statistic& moves) {
  std::vector<std::unique_ptr<Statistic>> statistics;
  statistics.push_back(rounds.clone());
  statistics.push_back(initialisations.clone());
  statistics.push_back(moves.clone());
  SearchStatistics stats = SearchStatistics(std::move(statistics));
  return stats;
}

SavedAssignment SearchProcedure::saveAssignment() const {
  return SavedAssignment(_assignment, _outputVarIds);
}

void SearchProcedure::tightenSearch() {
  if (_searchType == SearchType::PARALLEL) {
    _objective.tighten();
  } else {
    _objective.tighten(_savedAssignment->getCost());
    if (_searchType == SearchType::BEAMSEARCH)
      _assignment.setAssignment(_savedAssignment.value());
  }
}

void SearchProcedure::onAccepted() {
  // Prevent over-communication before an initial 0-violation solution
  // has been found
  if (!_hasSolution && _savedAssignment.has_value() &&
      _savedAssignment->getCost().isBetterThan(_assignment.getCost())) {
    return;
  }

  _savedAssignment = saveAssignment();

  bool isBest =
      _threadController->trySolution(_threadId, _savedAssignment.value());
  if (!isBest) {
    _savedAssignment = _threadController->solution();
  }

  if (!_hasSolution && _savedAssignment->getCost().getViolation() == 0) {
    _hasSolution = true;
  }

  tightenSearch();
}

Int SearchProcedure::run(SearchController& searchController,
                         std::unique_ptr<MetaHeuristic>&& metaHeuristic,
                         logging::Logger& logger) {
  do {
    logger.timedProcedure(logging::Level::LVL_TRACE, "initialize assignment",
                          [&] { _assignment.initialize(_random); });

    // TODO: handle this case: this should call some separate version
    if (_assignment.satisfiesConstraints()) onAccepted();

    metaHeuristic->start();

    while (searchController.shouldRun(_assignment) &&
           !metaHeuristic->isFinished()) {
      const auto cost = _assignment.performProbe(_random);
      if (metaHeuristic->acceptMove(cost)) {
        _assignment.commitLastProbe();
        if (!_hasSolution || _assignment.satisfiesConstraints()) {
          onAccepted();
        }
      }
    }
  } while (searchController.shouldRun(_assignment));

  return 1;
}

}  // namespace atlantis::search

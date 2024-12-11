#pragma once

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchController.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis::search {

/**
 * Search procedure based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class SearchProcedure {
 private:
  RandomProvider& _random;
  IAssignment& _assignment;
  neighborhoods::Neighborhood& _neighborhood;
  Objective _objective;

 public:
  SearchProcedure(RandomProvider& random, IAssignment& assignment,
                  neighborhoods::Neighborhood& neighborhood,
                  Objective objective)
      : _random(random),
        _assignment(assignment),
        _neighborhood(neighborhood),
        _objective(objective) {}

  SearchStatistics run(SearchController& controller, Annealer& annealer,
                       logging::Logger& logger);
};

}  // namespace atlantis::search

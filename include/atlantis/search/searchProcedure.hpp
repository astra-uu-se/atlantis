#pragma once

#include "atlantis/search/objective.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis::logging {
class Logger;
}

namespace atlantis::search {

class Annealer;
class RandomProvider;
class IAssignment;
class SearchController;
namespace neighborhoods {
class Neighborhood;
}

/**
 * Search procedure based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class SearchProcedure {
  RandomProvider& _random;
  IAssignment& _assignment;
  neighborhoods::Neighborhood& _neighborhood;
  Objective _objective;

 public:
  SearchProcedure(RandomProvider& random, IAssignment& assignment,
                  neighborhoods::Neighborhood& neighborhood,
                  const Objective& objective)
      : _random(random),
        _assignment(assignment),
        _neighborhood(neighborhood),
        _objective(objective) {}

  SearchStatistics run(SearchController& controller, Annealer& annealer,
                       logging::Logger& logger);
};

}  // namespace atlantis::search

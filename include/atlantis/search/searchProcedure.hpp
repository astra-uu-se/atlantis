#pragma once

#include "atlantis/search/objective.hpp"
#include "atlantis/search/searchStatistics.hpp"
#include "savedAssignment.hpp"

namespace atlantis::logging {
class Logger;
}

namespace atlantis::search {

class Annealer;
class RandomProvider;
class Assignment;
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
  Assignment& _assignment;
  neighborhoods::Neighborhood& _neighborhood;
  Objective _objective;
  std::optional<SavedAssignment> _solution;

 public:
  SearchProcedure(RandomProvider& random, Assignment& assignment,
                  neighborhoods::Neighborhood& neighborhood,
                  const Objective& objective)
      : _random(random),
        _assignment(assignment),
        _neighborhood(neighborhood),
        _objective(objective) {}

  int run(SearchController& controller, Annealer& annealer,
          logging::Logger& logger);
};

}  // namespace atlantis::search

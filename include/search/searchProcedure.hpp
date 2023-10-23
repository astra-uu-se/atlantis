#pragma once

#include "neighbourhoods/neighbourhood.hpp"
#include "objective.hpp"
#include "searchController.hpp"
#include "searchStatistics.hpp"
#include "logging/logger.hpp"

namespace atlantis::search {

/**
 * Search procedure based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class SearchProcedure {
 public:
  SearchProcedure(RandomProvider& random, Assignment& assignment,
                  neighbourhoods::Neighbourhood& neighbourhood,
                  Objective objective)
      : _random(random),
        _assignment(assignment),
        _neighbourhood(neighbourhood),
        _objective(std::move(objective)) {}

  SearchStatistics run(SearchController& controller, Annealer& annealer, logging::Logger& logger);

 private:
  RandomProvider& _random;
  Assignment& _assignment;
  neighbourhoods::Neighbourhood& _neighbourhood;
  Objective _objective;
};

}  // namespace search

#pragma once

#include <iostream>

#include "neighbourhoods/neighbourhood.hpp"
#include "searchController.hpp"
#include "solutionListener.hpp"

namespace search {

/**
 * Search procedure based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class SearchProcedure {
 public:
  SearchProcedure(RandomProvider& random, Assignment& assignment,
                  neighbourhoods::Neighbourhood& neighbourhood)
      : _random(random),
        _assignment(assignment),
        _neighbourhood(neighbourhood) {}

  void run(SearchController& controller, SolutionListener& solutionListener,
           Annealer& annealer);

 private:
  RandomProvider& _random;
  Assignment& _assignment;
  neighbourhoods::Neighbourhood& _neighbourhood;
};

}  // namespace search

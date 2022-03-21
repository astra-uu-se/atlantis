#pragma once

#include "neighbourhoods/neighbourhood.hpp"
#include "searchController.hpp"

namespace search {

/**
 * Search procedure based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class SearchProcedure {
 public:
  SearchProcedure(Assignment& assignment,
                  neighbourhoods::Neighbourhood* neighbourhood)
      : _assignment(assignment), _neighbourhood(neighbourhood) {}

  void run(SearchController& controller, Annealer* annealer);

 private:
  Assignment& _assignment;
  neighbourhoods::Neighbourhood* _neighbourhood;
};

}  // namespace search

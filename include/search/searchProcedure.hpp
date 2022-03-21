#pragma once

#include "neighbourhoods/neighbourhood.hpp"
#include "searchController.hpp"

namespace search {

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

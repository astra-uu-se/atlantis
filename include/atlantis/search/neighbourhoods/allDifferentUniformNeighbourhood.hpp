#pragma once

#include "atlantis/search/neighbourhoods/neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search::neighbourhoods {

class AllDifferentUniformNeighbourhood : public Neighbourhood {
 private:
  std::vector<search::SearchVar> _vars;
  std::vector<Int> _domain;
  bool _hasFreeValues;

 private:
  bool swapValues(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer);

  bool assignValue(RandomProvider& random, Assignment& assignment,
                   Annealer& annealer);

 public:
  AllDifferentUniformNeighbourhood(std::vector<search::SearchVar>&& vars,
                                   std::vector<Int>&& domain);

  void initialise(RandomProvider& random,
                  AssignmentModifier& modifications) override;

  bool randomMove(RandomProvider& random, Assignment& assignment,
                  Annealer& annealer) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighbourhoods

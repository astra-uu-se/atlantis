#pragma once

#include <vector>

#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/neighbourhoods//neighbourhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search::neighbourhoods {

class RandomNeighbourhood : public Neighbourhood {
 private:
  std::vector<SearchVar> _vars;

 public:
  RandomNeighbourhood(std::vector<SearchVar>&& vars);

  void initialize(RandomProvider&, IAssignment& assignment) override;

  size_t randomMove(RandomProvider&, IAssignment& assignment) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighbourhoods

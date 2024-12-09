#pragma once

#include <vector>

#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/neighborhoods//neighborhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search::neighborhoods {

class RandomNeighborhood : public Neighborhood {
 private:
  std::vector<SearchVar> _vars;

 public:
  RandomNeighborhood(std::vector<SearchVar>&& vars);

  void initialize(RandomProvider&, IAssignment& assignment) override;

  size_t randomMove(RandomProvider&, IAssignment& assignment) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighborhoods

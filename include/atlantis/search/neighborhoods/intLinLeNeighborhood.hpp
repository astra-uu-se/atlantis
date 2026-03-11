#pragma once

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {

class IntLinLeNeighborhood : public Neighborhood {
  std::vector<Int> _coeffs;
  std::vector<SearchVar> _vars;
    std::vector<size_t> _indices;
  Int _bound;
  Int _curSum;

 public:
  IntLinLeNeighborhood(std::vector<Int>&& coeffs, std::vector<SearchVar>&& vars,
                       Int bound);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

  void commitIf(const Assignment&) override;
};

}  // namespace atlantis::search::neighborhoods

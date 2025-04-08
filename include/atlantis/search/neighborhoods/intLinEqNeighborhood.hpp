#pragma once

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {

class IntLinEqNeighborhood : public Neighborhood {
  std::vector<Int> _coeffs;
  std::vector<SearchVar> _vars;
  Int _offset;
  std::vector<size_t> _indices;

 public:
  IntLinEqNeighborhood(std::vector<Int>&& coeffs, std::vector<SearchVar>&& vars,
                       Int offset);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighborhoods

#pragma once

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {
template <bool Boolean>
class BinaryLinLeNeighborhood : public Neighborhood {
  std::vector<Int> _coeffs;
  std::vector<SearchVar> _vars;
  std::vector<size_t> _indices;
  Int _bound;
  Int _curSum;
  Timestamp _curTimestamp;
  size_t _curVarIdx;

 public:
  BinaryLinLeNeighborhood(std::vector<Int>&& coeffs,
                          std::vector<SearchVar>&& vars, Int bound);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  void commitIf(const Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

template class BinaryLinLeNeighborhood<true>;
template class BinaryLinLeNeighborhood<false>;

}  // namespace atlantis::search::neighborhoods

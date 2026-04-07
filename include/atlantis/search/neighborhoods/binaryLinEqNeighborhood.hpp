#pragma once

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {
template <bool Violation>
class BinaryLinEqNeighborhood : public Neighborhood {
  std::vector<SearchVar> _vars;
  std::vector<size_t> _indices;
  size_t _numNegative;
  size_t _numNegTrue;
  size_t _numPosTrue;
  size_t _bound;
  Timestamp _curTimestamp;
  size_t _index1, _index2;

 public:
  BinaryLinEqNeighborhood(const std::vector<Int>& coeffs,
                          std::vector<SearchVar>&& vars, Int bound);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  void commitIf(const Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

template class BinaryLinEqNeighborhood<true>;
template class BinaryLinEqNeighborhood<false>;

}  // namespace atlantis::search::neighborhoods

#pragma once

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {

class AllDifferentUniformNeighborhood : public Neighborhood {
  std::vector<SearchVar> _vars;
  std::vector<Int> _freeVals;
  size_t _moveVarIdx;
  size_t _moveValIdx;
  Timestamp _curTimestamp;

 public:
  size_t swapValues(RandomProvider&, Assignment& assignment);

  size_t assignValue(RandomProvider&, Assignment& assignment);

  explicit AllDifferentUniformNeighborhood(std::vector<SearchVar>&& vars);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  void commitIf(const Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighborhoods

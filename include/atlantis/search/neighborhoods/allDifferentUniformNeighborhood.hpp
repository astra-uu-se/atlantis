#pragma once

#include <memory>

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search::neighborhoods {

class AllDifferentUniformNeighborhood : public Neighborhood {
 private:
  std::vector<search::SearchVar> _vars;
  std::vector<Int> _freeVals;
  size_t _moveVarIdx;
  size_t _moveValIdx;
  Timestamp _curTimestamp;

 public:
  size_t swapValues(RandomProvider&, IAssignment& assignment);

  size_t assignValue(RandomProvider&, IAssignment& assignment);

  AllDifferentUniformNeighborhood(std::vector<search::SearchVar>&& vars);

  void initialize(RandomProvider&, IAssignment&) override;

  size_t randomMove(RandomProvider&, IAssignment&) override;

  void commitIf(const IAssignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighborhoods

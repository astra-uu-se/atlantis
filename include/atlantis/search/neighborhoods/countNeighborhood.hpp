#pragma once

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {

class CountNeighborhood : public Neighborhood {
  std::vector<SearchVar> _vars;
  std::vector<size_t> _indices;
  Int _needle;
  size_t _amount;
  Timestamp _curTimestamp;
  size_t _index1, _index2;

 public:
  CountNeighborhood(std::vector<SearchVar>&& vars, Int needle, size_t amount);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  void commitIf(const Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }
};

}  // namespace atlantis::search::neighborhoods

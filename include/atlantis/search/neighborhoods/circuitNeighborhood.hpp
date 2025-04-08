#pragma once

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {
class SearchVar;
}

namespace atlantis::search::neighborhoods {

class CircuitNeighborhood : public Neighborhood {
  std::vector<SearchVar> _vars;
  Int _offset;

 public:
  explicit CircuitNeighborhood(std::vector<SearchVar>&&, Int offset);

  void initialize(RandomProvider&, Assignment&) override;

  size_t randomMove(RandomProvider&, Assignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

 private:
  [[nodiscard]] Int idx2Node(size_t nodeIdx) const noexcept;
  [[nodiscard]] size_t node2Idx(Int node) const noexcept;
};

}  // namespace atlantis::search::neighborhoods

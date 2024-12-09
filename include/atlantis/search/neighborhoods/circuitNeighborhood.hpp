#pragma once

#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search::neighborhoods {

class CircuitNeighborhood : public Neighborhood {
 private:
  std::vector<search::SearchVar> _vars;
  Int _offset;

 public:
  explicit CircuitNeighborhood(std::vector<search::SearchVar>&&, Int offset);

  void initialize(RandomProvider&, IAssignment&) override;

  size_t randomMove(RandomProvider&, IAssignment&) override;

  [[nodiscard]] const std::vector<SearchVar>& coveredVars() const override {
    return _vars;
  }

 private:
  [[nodiscard]] Int idx2Node(size_t nodeIdx) noexcept;
  [[nodiscard]] size_t node2Idx(Int node) noexcept;
};

}  // namespace atlantis::search::neighborhoods

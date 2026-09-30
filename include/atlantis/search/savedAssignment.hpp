#pragma once

#include "assignment.hpp"
#include "atlantis/invariantgraph/solverMapping.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/types.hpp"
#include "cost.hpp"

namespace atlantis::search {

class SavedAssignment {
  Cost _cost;
  std::vector<Int> _outputValues;
  std::vector<std::pair<propagation::VarId, Int>> _searchValues;

 public:
  explicit SavedAssignment(
      const Assignment& assignment,
      const std::vector<propagation::VarViewId>& outputVars);

  [[gnu::always_inline]] [[nodiscard]] Cost cost() const noexcept {
    return _cost;
  }

  [[nodiscard]] const std::vector<Int>& outputValues() const noexcept {
    return _outputValues;
  }

  [[nodiscard]] const std::vector<std::pair<propagation::VarId, Int>>&
  searchValues() const noexcept {
    return _searchValues;
  }

  void setCost(const Cost& cost) { _cost = cost; }
};

}  // namespace atlantis::search
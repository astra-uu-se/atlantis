#pragma once

#include <unordered_map>

#include "assignment.hpp"
#include "atlantis/types.hpp"
#include "cost.hpp"

namespace atlantis::search {

class SavedAssignment {
  Cost _cost;
  std::vector<Int> _outputValues;

 public:
  explicit SavedAssignment(const Assignment &assignment,
    const std::vector<propagation::VarViewId>& outputVars)
  : _cost(assignment.getCost()),
    _outputValues(outputVars.size()) {
    for (std::size_t i = 0; i < outputVars.size(); ++i) {
      _outputValues[i] = assignment.committedValue(outputVars[i]);
    }
  }

  [[gnu::always_inline]] [[nodiscard]] Cost getCost() const { return _cost; }

  [[nodiscard]] const std::vector<Int>& getOutputValues() const { return _outputValues; }

  void setCost(const Cost &cost) { _cost = cost; }
};

}  // namespace atlantis::search
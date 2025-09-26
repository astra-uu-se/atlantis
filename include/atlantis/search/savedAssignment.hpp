#pragma once

#include <unordered_map>

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
      const std::vector<propagation::VarViewId>& outputVars)
      : _cost(assignment.getCost()),
        _outputValues(outputVars.size()),
        _searchValues(assignment.searchVars().size()) {
    for (std::size_t i = 0; i < outputVars.size(); ++i) {
      _outputValues[i] = assignment.committedValue(outputVars[i]);
    }
    for (size_t i = 0; i < assignment.searchVars().size(); ++i) {
      _searchValues[i] = {assignment.searchVars()[i],
                          assignment.currentValue(assignment.searchVars()[i])};
    }
  }

  [[gnu::always_inline]] [[nodiscard]] Cost getCost() const { return _cost; }

  [[nodiscard]] const std::vector<Int>& getOutputValues() const {
    return _outputValues;
  }

  [[nodiscard]] const std::vector<std::pair<propagation::VarId, Int>>&
  getSearchValues() const {
    return _searchValues;
  }

  void setCost(const Cost& cost) { _cost = cost; }
};

}  // namespace atlantis::search
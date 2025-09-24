#pragma once

#include <unordered_map>

#include "assignment.hpp"
#include "atlantis/types.hpp"
#include "cost.hpp"

namespace atlantis::search {

class SavedAssignment {
  Cost _cost;
  std::unordered_map<propagation::VarId, Int> _values;

 public:
  explicit SavedAssignment(const Assignment &assignment)
      : _cost(assignment.getCost()), _values(assignment.currentValues()) {}

  [[gnu::always_inline]] [[nodiscard]] Cost getCost() const { return _cost; }

  [[gnu::always_inline]] [[nodiscard]]
  std::unordered_map<propagation::VarId, Int> getValues() {
    return _values;
  }
};

}  // namespace atlantis::search
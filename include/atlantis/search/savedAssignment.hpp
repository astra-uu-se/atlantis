#pragma once

#include "assignment.hpp"
#include "atlantis/types.hpp"
#include "cost.hpp"

namespace atlantis::search {

class SavedAssignment {
  Cost _cost;
  std::vector<Int> _values;

 public:
  explicit SavedAssignment(const Assignment &assignment)
  : _cost(assignment.getCost()),
    _values(assignment.currentValues()) {}

  [[nodiscard]] Cost getCost() const { return _cost; }

  [[nodiscard]] std::vector<Int> getValues() { return _values; }

  void setCost(const Cost &cost) { _cost = cost; }
};

}  // namespace atlantis::search
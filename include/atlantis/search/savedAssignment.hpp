#pragma once

#include "assignment.hpp"
#include "atlantis/types.hpp"
#include "cost.hpp"
#include "searchStatistics.hpp"

namespace atlantis::search {

class SavedAssignment {
  Cost _cost;
  // SearchStatistics _statistics;
  std::vector<Int> _values;

 public:
  explicit SavedAssignment(const Assignment &assignment);

  [[nodiscard]] Cost getCost() const { return _cost; }

  // TODO: Printing functions?
  // Can probably be more or less copied from fznBackend, but will require
  // additional data to be stored.
};

}  // namespace atlantis::search
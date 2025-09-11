#pragma once

#include "assignment.hpp"
#include "atlantis/types.hpp"
#include "cost.hpp"
#include "searchStatistics.hpp"

namespace atlantis::search {

class SavedAssignment {
  Cost _cost;
  std::vector<Int> _values;

 public:
  explicit SavedAssignment(const Assignment &assignment);

  [[nodiscard]] Cost getCost() const { return _cost; }
};

}  // namespace atlantis::search
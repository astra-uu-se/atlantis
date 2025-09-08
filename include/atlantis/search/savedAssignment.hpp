#pragma once
#include <unordered_map>

#include "assignment.hpp"
#include "atlantis/types.hpp"
#include "cost.hpp"
#include "searchStatistics.hpp"

namespace atlantis::search {

class SavedAssignment {
  Cost _cost;
  std::unordered_map<std::string_view, std::string>
      _statistics;  // TODO: Consider switching this to be a SearchStatistic
                    // type. That would be less efficient but probably more
                    // practical.
  std::vector<Int> _values;

 public:
  SavedAssignment(const Assignment &assignment,
                  std::unordered_map<std::string_view, std::string> statistics);

  // TODO: Printing functions?
  // Can probably be more or less copied from fznBackend, but will require
  // additional data to be stored.
};

}  // namespace atlantis::search
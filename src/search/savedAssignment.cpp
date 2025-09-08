#include "atlantis/search/savedAssignment.hpp"

#include <memory>

#include "atlantis/search/cost.hpp"

namespace atlantis::search {

SavedAssignment::SavedAssignment(const Assignment &assignment,
                                 std::unordered_map<std::string_view, std::string> statistics)
    : _cost(assignment.getCost()),  // TODO: double check that this is a copy
      _statistics(statistics),
      _values(assignment.currentValues()) {}

}  // namespace atlantis::search

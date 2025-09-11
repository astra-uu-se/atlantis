#include "atlantis/search/savedAssignment.hpp"

#include "atlantis/search/cost.hpp"

namespace atlantis::search {

SavedAssignment::SavedAssignment(const Assignment &assignment)
    : _cost(assignment.getCost()),  // TODO: double check that this is a copy
      _values(assignment.currentValues())
{}

}  // namespace atlantis::search

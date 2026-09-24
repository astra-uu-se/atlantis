#include "atlantis/search/savedAssignment.hpp"

namespace atlantis::search {

SavedAssignment::SavedAssignment(
    const Assignment& assignment,
    const std::vector<propagation::VarViewId>& outputVars)
    : _cost(assignment.cost()),
      _outputValues(outputVars.size()),
      _searchValues(assignment.searchVars().size()) {
  for (std::size_t i = 0; i < outputVars.size(); ++i) {
    _outputValues[i] = assignment.committedValue(outputVars[i]);
  }
  for (size_t i = 0; i < assignment.searchVars().size(); ++i) {
    _searchValues[i] = {assignment.searchVars()[i],
                        assignment.currentValue(propagation::VarViewId(
                            assignment.searchVars()[i]))};
  }
}
}  // namespace atlantis::search

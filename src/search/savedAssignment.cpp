#include "atlantis/search/savedAssignment.hpp"

#include "atlantis/search/cost.hpp"

namespace atlantis::search {

SavedAssignment::SavedAssignment(const Assignment &assignment)
    : _cost(assignment.getCost()),  // TODO: double check that this is a copy
                                    // _statistics(std::move(statistics)),
                                    // _statistics(SearchStatistics()),
                                    // _statistics(statistics),
      _values(assignment.currentValues())
// _values({})
{}

}  // namespace atlantis::search

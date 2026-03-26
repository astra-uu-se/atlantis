#include "atlantis/search/neighborhoods/tableNeighborhood.hpp"

#include <algorithm>
#include <cassert>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search::neighborhoods {

TableNeighborhood::TableNeighborhood(std::vector<SearchVar>&& vars,
                                     std::vector<std::vector<Int>>&& table)
    : _vars(std::move(vars)),
      _table(std::move(table)),
      _index({NULL_TIMESTAMP, -1, -1}) {
  assert(_vars.size() > 1);
}

void TableNeighborhood::initialize(RandomProvider& random,
                                   Assignment& assignment) {
  const Int row = random.intInRange(0, static_cast<Int>(_table.size()) - 1);
  _index.setValue(assignment.currentTimestamp(), row);
  _index.commit();

  for (size_t c = 0; c < _vars.size(); ++c) {
    assignment.set(_vars[c].solverId(), _table[row][c]);
  }
}

size_t TableNeighborhood::randomMove(RandomProvider& random,
                                     Assignment& assignment) {
  const Int row = random.intInRange(0, static_cast<Int>(_table.size()) - 1,
                                    _index.committedValue());
  _index.setValue(assignment.currentTimestamp(), row);

  Int numModified = 0;
  for (size_t c = 0; c < _vars.size(); ++c) {
    assignment.set(_vars[c].solverId(), _table[row][c]);
    numModified += assignment.committedValue(_vars[c].solverId()) !=
                           assignment.currentValue(_vars[c].solverId())
                       ? 1
                       : 0;
  }
  return numModified;
}

void TableNeighborhood::commitIf(const Assignment& assignment) {
  _index.commitIf(assignment.currentTimestamp());
}

}  // namespace atlantis::search::neighborhoods

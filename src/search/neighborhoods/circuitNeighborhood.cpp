#include "atlantis/search/neighborhoods/circuitNeighborhood.hpp"

#include <algorithm>
#include <array>

#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search::neighborhoods {

CircuitNeighborhood::CircuitNeighborhood(std::vector<SearchVar>&& vars,
                                         Int offset)
    : _vars(std::move(vars)), _offset(offset) {}

void CircuitNeighborhood::initialize(RandomProvider& random,
                                     IAssignment& assignment) {
  Int numAvailable = static_cast<Int>(_vars.size());
  std::vector<bool> idxIsAvailable(_vars.size(), true);

  for (auto& var : _vars) {
    if (var.isFixed()) {
      const auto nextNode = var.domain()->lowerBound();
      const auto nextNodeIdx = node2Idx(nextNode);

      assert(idxIsAvailable.at(nextNodeIdx));

      assignment.set(var.solverId(), nextNode);
      idxIsAvailable[nextNodeIdx] = false;
      --numAvailable;
    }
  }

  if (numAvailable == 0) {
    return;
  }

  std::vector<size_t> availableIndices;
  availableIndices.reserve(numAvailable);
  for (size_t i = 0; i < idxIsAvailable.size(); ++i) {
    if (idxIsAvailable[i]) {
      availableIndices.emplace_back(i);
    }
  }

  const auto j = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(availableIndices.size() - 1)));
  assert(j < availableIndices.size());
  std::swap(availableIndices[0], availableIndices[j]);
  size_t curNodeIdx = availableIndices[0];

  for (Int i = 1; i < static_cast<Int>(availableIndices.size()); ++i) {
    assert(curNodeIdx < _vars.size());
    while (_vars[curNodeIdx].isFixed()) {
      curNodeIdx = node2Idx(_vars[curNodeIdx].domain()->lowerBound());
      assert(std::ranges::none_of(
          availableIndices.begin(), availableIndices.end(),
          [&](const size_t idx) { return idx == curNodeIdx; }));
    }

    const auto k = static_cast<size_t>(
        random.intInRange(i, static_cast<Int>(availableIndices.size() - 1)));
    assert(k < availableIndices.size());
    std::swap(availableIndices[i], availableIndices[k]);
    const size_t nextNodeIdx = availableIndices[i];

    assert(nextNodeIdx < _vars.size());

    assignment.set(_vars[curNodeIdx].solverId(), idx2Node(nextNodeIdx));

    curNodeIdx = nextNodeIdx;
  }

  assignment.set(_vars[curNodeIdx].solverId(), idx2Node(availableIndices[0]));
}

static size_t determineNewNext(RandomProvider& random, size_t node,
                               size_t oldNext, size_t numVars) {
  assert(numVars >= 3);
  // Based on https://stackoverflow.com/a/39631885.
  // Note: Random.next(n) returns an integer between 0..n-1
  const std::array<size_t, 2> excluded{std::min(node, oldNext),
                                       std::max(node, oldNext)};
  // the range is between 0..numVars-1
  // excluded.size() = 2
  auto newNext =
      static_cast<size_t>(random.intInRange(0, static_cast<Int>(numVars - 3)));
  for (const auto num : excluded) {
    if (newNext < num) {
      return newNext;
    }

    newNext++;
  }

  return newNext;
}

size_t CircuitNeighborhood::randomMove(RandomProvider& random,
                                       IAssignment& assignment) {
  auto nodeIdx = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(_vars.size() - 1)));
  auto oldNextIdx =
      node2Idx(assignment.committedValue(_vars[nodeIdx].solverId()));

  auto newNextIdx = determineNewNext(random, nodeIdx, oldNextIdx, _vars.size());
  assert(newNextIdx < _vars.size());
  const auto kIdx =
      node2Idx(assignment.committedValue(_vars[oldNextIdx].solverId()));
  const auto lastIdx =
      node2Idx(assignment.committedValue(_vars[newNextIdx].solverId()));

  for (const auto varIdx : {nodeIdx, oldNextIdx, newNextIdx}) {
    if (_vars[varIdx].isFixed()) {
      return 0;
    }
  }
  assignment.set(_vars[nodeIdx].solverId(), idx2Node(kIdx));
  assignment.set(_vars[oldNextIdx].solverId(), idx2Node(lastIdx));
  assignment.set(_vars[newNextIdx].solverId(), idx2Node(oldNextIdx));
  return 3;
}

Int CircuitNeighborhood::idx2Node(size_t nodeIdx) const noexcept {
  // Account for index sets starting at _offset instead of 0.
  return static_cast<Int>(nodeIdx) + _offset;
}

size_t CircuitNeighborhood::node2Idx(Int node) const noexcept {
  // Account for index sets starting at _offset instead of 0.
  assert(node >= _offset);
  return static_cast<size_t>(node - _offset);
}

}  // namespace atlantis::search::neighborhoods

#include "atlantis/search/neighbourhoods/circuitNeighbourhood.hpp"

#include <algorithm>

namespace atlantis::search::neighbourhoods {

CircuitNeighbourhood::CircuitNeighbourhood(std::vector<SearchVar>&& vars,
                                           Int offset)
    : _vars(std::move(vars)), _offset(offset) {}

void CircuitNeighbourhood::initialise(RandomProvider& random,
                                      AssignmentModifier& modifications) {
  Int numAvailable = _vars.size();
  std::vector<bool> idxIsAvailable(_vars.size(), true);

  for (auto& var : _vars) {
    if (var.isFixed()) {
      auto nextNode = var.constDomain().lowerBound();
      auto nextNodeIdx = node2Idx(nextNode);

      assert(idxIsAvailable.at(nextNodeIdx));

      modifications.set(var.solverId(), nextNode);
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

  const size_t j = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(availableIndices.size() - 1)));
  assert(j < availableIndices.size());
  std::swap(availableIndices[0], availableIndices[j]);
  size_t curNodeIdx = availableIndices[0];

  for (size_t i = 1; i < availableIndices.size(); ++i) {
    assert(curNodeIdx < _vars.size());
    while (_vars[curNodeIdx].isFixed()) {
      curNodeIdx = node2Idx(_vars[curNodeIdx].constDomain().lowerBound());
      assert(std::none_of(availableIndices.begin(), availableIndices.end(),
                          [&](const size_t idx) { return idx == curNodeIdx; }));
    }

    const size_t j = static_cast<size_t>(
        random.intInRange(i, static_cast<Int>(availableIndices.size() - 1)));
    assert(j < availableIndices.size());
    std::swap(availableIndices[i], availableIndices[j]);
    const size_t nextNodeIdx = availableIndices[i];

    assert(nextNodeIdx < _vars.size());

    modifications.set(_vars[curNodeIdx].solverId(), idx2Node(nextNodeIdx));

    curNodeIdx = nextNodeIdx;
  }

  modifications.set(_vars[curNodeIdx].solverId(),
                    idx2Node(availableIndices[0]));
}

static size_t determineNewNext(RandomProvider& random, size_t node,
                               size_t oldNext, size_t numVars) {
  assert(numVars >= 3);
  // Based on https://stackoverflow.com/a/39631885.
  // Note: Random.next(n) returns an integer between 0..n-1
  std::array<size_t, 2> excluded{std::min(node, oldNext),
                                 std::max(node, oldNext)};
  // the range is between 0..numVars-1
  // excluded.size() = 2
  size_t newNext =
      static_cast<size_t>(random.intInRange(0, static_cast<Int>(numVars - 3)));
  for (const auto num : excluded) {
    if (newNext < num) {
      return newNext;
    }

    newNext++;
  }

  return newNext;
}

bool CircuitNeighbourhood::randomMove(RandomProvider& random,
                                      Assignment& assignment,
                                      Annealer& annealer) {
  auto nodeIdx = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(_vars.size() - 1)));
  auto oldNextIdx = node2Idx(assignment.value(_vars[nodeIdx].solverId()));

  auto newNextIdx = determineNewNext(random, nodeIdx, oldNextIdx, _vars.size());
  assert(newNextIdx < _vars.size());
  auto kIdx = node2Idx(assignment.value(_vars[oldNextIdx].solverId()));
  auto lastIdx = node2Idx(assignment.value(_vars[newNextIdx].solverId()));

  for (auto varIdx : {nodeIdx, oldNextIdx, newNextIdx}) {
    if (_vars[varIdx].isFixed()) {
      return false;
    }
  }

  Move<3> move({_vars[nodeIdx].solverId(), _vars[oldNextIdx].solverId(),
                _vars[newNextIdx].solverId()},
               {idx2Node(kIdx), idx2Node(lastIdx), idx2Node(oldNextIdx)});
  return annealer.acceptMove(move);
}

Int CircuitNeighbourhood::idx2Node(size_t nodeIdx) noexcept {
  // Account for index sets starting at _offset instead of 0.
  return static_cast<Int>(nodeIdx) + _offset;
}

size_t CircuitNeighbourhood::node2Idx(Int node) noexcept {
  // Account for index sets starting at _offset instead of 0.
  assert(node >= _offset);
  return static_cast<size_t>(node - _offset);
}

}  // namespace atlantis::search::neighbourhoods

#include "search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::search::neighbourhoods {

CircuitNeighbourhood::CircuitNeighbourhood(std::vector<SearchVar>&& vars)
    : _vars(std::move(vars)) {}

void CircuitNeighbourhood::initialise(RandomProvider& random,
                                      AssignmentModifier& modifications) {
  std::vector<Int> availableIndices(_vars.size());
  std::iota(availableIndices.begin(), availableIndices.end(), 0);

  auto getValue = [](SearchVar& var) { return var.domain().lowerBound(); };

  for (auto& var : _vars) {
    if (var.isFixed()) {
      auto nextNode = getValue(var);
      auto nextNodeIdx = getNodeIdx(nextNode);

      auto it = std::find(availableIndices.begin(), availableIndices.end(),
                          nextNodeIdx);
      assert(it != availableIndices.end());

      modifications.set(var.solverId(), nextNode);
      availableIndices.erase(it);
    }
  }

  auto firstIt =
      random.iterator(availableIndices.begin(), availableIndices.end());
  size_t firstNodeIdx = *firstIt;
  availableIndices.erase(firstIt);

  size_t currentNodeIdx = firstNodeIdx;
  while (!availableIndices.empty()) {
    if (_vars[currentNodeIdx].isFixed()) {
      currentNodeIdx = getNodeIdx(getValue(_vars[currentNodeIdx]));
      continue;
    }

    auto nextIt =
        random.iterator(availableIndices.begin(), availableIndices.end());
    assert(nextIt < availableIndices.end());

    size_t nextNodeIdx = *nextIt;
    assert(nextNodeIdx < _vars.size());

    modifications.set(_vars[currentNodeIdx].solverId(), getNode(nextNodeIdx));

    availableIndices.erase(nextIt);
    currentNodeIdx = nextNodeIdx;
  }

  modifications.set(_vars[currentNodeIdx].solverId(), getNode(firstNodeIdx));
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

bool CircuitNeighbourhood::randomMove(RandomProvider& random,
                                      Assignment& assignment,
                                      Annealer& annealer) {
  auto nodeIdx = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(_vars.size() - 1)));
  auto oldNextIdx = getNodeIdx(assignment.value(_vars[nodeIdx].solverId()));

  auto newNextIdx = determineNewNext(random, nodeIdx, oldNextIdx, _vars.size());
  assert(newNextIdx < _vars.size());
  auto kIdx = getNodeIdx(assignment.value(_vars[oldNextIdx].solverId()));
  auto lastIdx = getNodeIdx(assignment.value(_vars[newNextIdx].solverId()));

  for (auto varIdx : {nodeIdx, oldNextIdx, newNextIdx}) {
    if (_vars[varIdx].isFixed()) {
      return false;
    }
  }

  Move<3> move({_vars[nodeIdx].solverId(), _vars[oldNextIdx].solverId(),
                _vars[newNextIdx].solverId()},
               {getNode(kIdx), getNode(lastIdx), getNode(oldNextIdx)});
  return annealer.acceptMove(move);
}

Int CircuitNeighbourhood::getNode(size_t nodeIdx) noexcept {
  // Account for index sets starting at 1 instead of 0.
  return static_cast<Int>(nodeIdx) + 1;
}

size_t CircuitNeighbourhood::getNodeIdx(Int node) noexcept {
  // Account for index sets starting at 1 instead of 0.
  return static_cast<size_t>(node) - 1;
}

}  // namespace atlantis::search::neighbourhoods
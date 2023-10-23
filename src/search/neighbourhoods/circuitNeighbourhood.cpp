#include "search/neighbourhoods/circuitNeighbourhood.hpp"

#include <algorithm>

namespace atlantis::search::neighbourhoods {

CircuitNeighbourhood::CircuitNeighbourhood(
    std::vector<SearchVariable>&& variables)
    : _variables(std::move(variables)) {}

void CircuitNeighbourhood::initialise(RandomProvider& random,
                                      AssignmentModifier& modifications) {
  std::vector<Int> availableIndices(_variables.size());
  std::iota(availableIndices.begin(), availableIndices.end(), 0);

  auto getValue = [](SearchVariable& variable) {
    return variable.domain().lowerBound();
  };

  for (auto& variable : _variables) {
    if (variable.isFixed()) {
      auto nextNode = getValue(variable);
      auto nextNodeIdx = getNodeIdx(nextNode);

      auto it = std::find(availableIndices.begin(), availableIndices.end(),
                          nextNodeIdx);
      assert(it != availableIndices.end());

      modifications.set(variable.solverId(), nextNode);
      availableIndices.erase(it);
    }
  }

  auto firstIt =
      random.iterator(availableIndices.begin(), availableIndices.end());
  size_t firstNodeIdx = *firstIt;
  availableIndices.erase(firstIt);

  size_t currentNodeIdx = firstNodeIdx;
  while (!availableIndices.empty()) {
    if (_variables[currentNodeIdx].isFixed()) {
      currentNodeIdx = getNodeIdx(getValue(_variables[currentNodeIdx]));
      continue;
    }

    auto nextIt =
        random.iterator(availableIndices.begin(), availableIndices.end());
    assert(nextIt < availableIndices.end());

    size_t nextNodeIdx = *nextIt;
    assert(nextNodeIdx < _variables.size());

    modifications.set(_variables[currentNodeIdx].solverId(),
                      getNode(nextNodeIdx));

    availableIndices.erase(nextIt);
    currentNodeIdx = nextNodeIdx;
  }

  modifications.set(_variables[currentNodeIdx].solverId(),
                    getNode(firstNodeIdx));
}

static size_t determineNewNext(RandomProvider& random, size_t node,
                               size_t oldNext, size_t numVariables) {
  assert(numVariables >= 3);
  // Based on https://stackoverflow.com/a/39631885.
  // Note: Random.next(n) returns an integer between 0..n-1
  std::array<size_t, 2> excluded{std::min(node, oldNext),
                                 std::max(node, oldNext)};
  // the range is between 0..numVariables-1
  // excluded.size() = 2
  auto newNext = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(numVariables - 3)));
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
      random.intInRange(0, static_cast<Int>(_variables.size() - 1)));
  auto oldNextIdx =
      getNodeIdx(assignment.value(_variables[nodeIdx].solverId()));

  auto newNextIdx =
      determineNewNext(random, nodeIdx, oldNextIdx, _variables.size());
  assert(newNextIdx < _variables.size());
  auto kIdx = getNodeIdx(assignment.value(_variables[oldNextIdx].solverId()));
  auto lastIdx =
      getNodeIdx(assignment.value(_variables[newNextIdx].solverId()));

  for (auto varIdx : {nodeIdx, oldNextIdx, newNextIdx}) {
    if (_variables[varIdx].isFixed()) {
      return false;
    }
  }

  Move<3> move(
      {_variables[nodeIdx].solverId(), _variables[oldNextIdx].solverId(),
       _variables[newNextIdx].solverId()},
      {getNode(kIdx), getNode(lastIdx), getNode(oldNextIdx)});
  return annealer.acceptMove(move);
}

Int CircuitNeighbourhood::getNode(size_t nodeIdx) const noexcept {
  // Account for index sets starting at 1 instead of 0.
  return static_cast<Int>(nodeIdx) + 1;
}

size_t CircuitNeighbourhood::getNodeIdx(Int node) const noexcept {
  // Account for index sets starting at 1 instead of 0.
  return static_cast<size_t>(node) - 1;
}

}  // namespace atlantis::search::neighbourhoods
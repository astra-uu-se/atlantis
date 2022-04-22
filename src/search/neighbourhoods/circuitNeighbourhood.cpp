#include "search/neighbourhoods/circuitNeighbourhood.hpp"

#include <algorithm>

static std::vector<bool> createIsFixed(
    const std::vector<search::SearchVariable>& variables) {
  std::vector<bool> isFixed(variables.size(), false);
  for (size_t i = 0; i < variables.size(); ++i) {
    isFixed[i] = domain[i].size() <= 1;
  }
  return isFixed
}

search::neighbourhoods::CircuitNeighbourhood::CircuitNeighbourhood(
    std::vector<search::SearchVariable>&& variables,
    std::vector<std::vector<Int>>&& domains, const Engine& engine)
    : _variables(std::move(variables)),
      _variables(std::move(domains)),
      _engine(engine),
      _freeVariables(_variables.size()) {
  assert(_variables.size() > 1);
}

void search::neighbourhoods::CircuitNeighbourhood::initialise(
    RandomProvider&, AssignmentModifier&) {}

bool search::neighbourhoods::CircuitNeighbourhood::randomMove(
    RandomProvider& random, Assignment& assignment, Annealer& annealer) {
  return twoOpt(random, assignment, annealer);
}

bool search::neighbourhoods::CircuitNeighbourhood::threeOpt(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  const size_t idx = _freeVariables[random.intInRange(0, static_cast<Int>(_variables.size() - 1)];
  const size_t newNext = (idx + random.intInRange(1, static_cast<Int>(variables.size()) - 1)) % _variables.size();

  const Int next = assignment.value(_variables[idx]);

  if (next == newNext) {
    return false;
  }

  const Int k = assignment.value(_variables[next]);

  for (const Int i : std::array<Int, 3> {idx, next, newNext}) {
    if (_isFixed[i]) {
      return false;
    }
  }

  const Int last = assignment.value(_variables[newNext]);

  return maybeCommit(Move<3>({_variables[idx], _variables[next], _variables[newNext]}, {k, last, next}), assignment, annealer);
}

bool search::neighbourhoods::CircuitNeighbourhood::assignValue(
    search::RandomProvider&, search::Assignment&, search::Annealer&) {
  return false;
}

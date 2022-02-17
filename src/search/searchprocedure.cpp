#include "search/searchprocedure.hpp"

template <typename Neighbourhood>
void search::SearchProcedure<Neighbourhood>::run(SearchContext& context) {
  while (!context.shouldStop(_assignment)) {
    _neighbourhood.initialize(_assignment.engine());

    while (_annealer.globalCondition() && !context.shouldStop(_assignment)) {
      while (_annealer.searchLocally() && !context.shouldStop(_assignment)) {
        Move m = _neighbourhood.randomMove(_assignment.engine());

        if (accept(m)) {
          _assignment.commitMove(m);
        }
      }
    }
  }
}

template <typename Neighbourhood>
bool search::SearchProcedure<Neighbourhood>::accept(Move m) {
  Int newObjectiveValue = evaluate(_assignment.probeMove(m));

  return _annealer.accept(evaluate(_assignment.objective()), newObjectiveValue);
}

template <typename Neighbourhood>
Int search::SearchProcedure<Neighbourhood>::evaluate(
    Objective objective) const {
  return objective.evaluate(_violationsWeight, _modelObjectiveWeight);
}

template class search::SearchProcedure<
    search::neighbourhoods::MaxViolatingNeighbourhood>;
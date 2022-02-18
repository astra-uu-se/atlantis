#include "search/searchprocedure.hpp"

#include <iostream>

static void printMove(
    const std::map<VarId, std::shared_ptr<fznparser::Variable>>& variableMap,
    const search::Move& move) {
  if (move.targets().empty()) {
    std::cout << "{}" << std::endl;
    return;
  }

  std::cout << "{ ";
  for (size_t idx = 0; idx < move.targets().size(); ++idx) {
    std::cout << "(" << variableMap.at(move.targets()[idx])->name() << " <- "
              << move.values()[idx] << ")";
  }
  std::cout << " }" << std::endl;
}

template <typename Neighbourhood>
void search::SearchProcedure<Neighbourhood>::run(
    SearchContext& context,
    const std::map<VarId, std::shared_ptr<fznparser::Variable>>& variableMap) {
  _assignment.initialise(_neighbourhood);

  while (_annealer.globalCondition() && !context.shouldStop(_assignment)) {
    while (_annealer.searchLocally() && !context.shouldStop(_assignment)) {
      Move m = _neighbourhood.randomMove(_assignment.engine());
      printMove(variableMap, m);

      if (accept(m)) {
        _assignment.commitMove(m);
        std::cout << "ACCEPTED" << std::endl;
      }
    }
  }

  if (_assignment.satisfiesConstraints()) {
    std::cout << "Found solution:" << std::endl;

    for (auto varIdBase : _assignment.engine().getDecisionVariables()) {
      auto varId = VarId(varIdBase.id);

      if (variableMap.count(varId) == 0) {
        continue;
      }

      std::cout << "\t" << variableMap.at(varId)->name() << " = "
                << _assignment.engine().getCommittedValue(varId) << std::endl;
    }
  } else {
    std::cout << "Couldn't find solution" << std::endl;
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
#include "search/assignment.hpp"

search::Assignment::Assignment(PropagationEngine& engine, VarId& violation,
                               VarId& objective)
    : _engine(engine), _violation(violation), _objective(objective) {
  _searchVariables.reserve(engine.searchVariables().size());
  for (const auto& varIdBase : engine.searchVariables()) {
    VarId varId(varIdBase.id);
    if (engine.lowerBound(varId) != engine.upperBound(varId)) {
      _searchVariables.push_back(varId);
    }
  }
}

Int search::Assignment::value(VarId var) const noexcept {
  return _engine.committedValue(var);
}

bool search::Assignment::satisfiesConstraints() const noexcept {
  return _engine.committedValue(_violation) == 0;
}

search::Cost search::Assignment::cost() const noexcept {
  return {_engine.committedValue(_violation),
          _engine.committedValue(_objective)};
}

#include "search/variableStore.hpp"

search::VariableStore::VariableStore(
    const PropagationEngine& engine,
    const VariableStore::VariableMap& variables)
    : _engine(engine) {
  IdBase largestId = 0;
  for (const auto& [variable, _] : variables)
    largestId = std::max(variable.id, largestId);

  _domains.resize(largestId + 1);
  for (const auto& [engineVariable, modelVariable] : variables) {
    auto pos = _domains.cbegin() + engineVariable.id;
    _domains.insert(pos,
                          Domain{modelVariable->domain()->lowerBound(),
                                 modelVariable->domain()->upperBound()});
  }
}

search::VariableStore::Domain search::VariableStore::domain(
    VarId variable) {
  if (variable.id < _domains.size() && _domains[variable.id]) {
    return *_domains[variable.id];
  }

  return {_engine.lowerBound(variable), _engine.upperBound(variable)};
}

#include "invariants/invariant.hpp"

#include "core/propagationEngine.hpp"

void Invariant::notify(LocalId id) { _modifiedVars.push(id); }

void Invariant::compute(Timestamp t, Engine& e) {
  assert(_modifiedVars.size() > 0);
  assert(_primaryOutput != NULL_ID);

  while (_modifiedVars.hasNext()) {
    // don't turn this into a for loop...
    LocalId toNotify = _modifiedVars.pop();
    this->notifyIntChanged(t, e, toNotify);
  }
}

void Invariant::registerDefinedVariable(Engine& e, VarId v) {
  if (_primaryOutput == NULL_ID) {
    _primaryOutput = v;
  } else {
    _outputVars.push_back(v);
  }
  e.registerDefinedVariable(v, _id);
}

void Invariant::updateValue(Timestamp t, Engine& e, VarId id, Int val) {
  e.updateValue(t, id, val);
}

void Invariant::incValue(Timestamp t, Engine& e, VarId id, Int val) {
  e.incValue(t, id, val);
}

void Invariant::queueNonPrimaryOutputVarsForPropagation(Timestamp t,
                                                        Engine& e) {
  for (VarId outputVarId : _outputVars) {
    if (!e.hasChanged(t, outputVarId)) {
      continue;
    }
    e.queueForPropagation(t, outputVarId);
  }
}
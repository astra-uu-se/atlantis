#include "invariants/invariant.hpp"

#include "core/propagationEngine.hpp"

void Invariant::notify(LocalId id) { _modifiedVars.push(id); }

void Invariant::compute(Timestamp ts, Engine& engine) {
  assert(_modifiedVars.size() > 0);
  assert(_primaryOutput != NULL_ID);

  while (_modifiedVars.hasNext()) {
    // don'ts turn this into a for loop...
    LocalId toNotify = _modifiedVars.pop();
    this->notifyIntChanged(ts, engine, toNotify);
  }
}

void Invariant::registerDefinedVariable(Engine& engine, VarId id) {
  if (_primaryOutput == NULL_ID) {
    _primaryOutput = id;
  } else {
    _outputVars.push_back(id);
  }
  engine.registerDefinedVariable(id, _id);
}

void Invariant::updateValue(Timestamp ts, Engine& engine, VarId id, Int val) {
  engine.updateValue(ts, id, val);
}

void Invariant::incValue(Timestamp ts, Engine& engine, VarId id, Int val) {
  engine.incValue(ts, id, val);
}

void Invariant::queueNonPrimaryOutputVarsForPropagation(Timestamp ts,
                                                        Engine& engine) {
  for (VarId outputVarId : _outputVars) {
    if (!engine.hasChanged(ts, outputVarId)) {
      continue;
    }
    engine.queueForPropagation(ts, outputVarId);
  }
}
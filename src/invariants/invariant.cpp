#include "invariants/invariant.hpp"

#include "core/propagationEngine.hpp"

void Invariant::notify(LocalId id) { _modifiedVars.push(id); }

void Invariant::compute(Timestamp ts, Engine& engine) {
  assert(_modifiedVars.size() > 0);
  assert(_primaryDefinedVar != NULL_ID);

  while (_modifiedVars.hasNext()) {
    // don't turn this into a for loop...
    this->notifyInputChanged(ts, engine, _modifiedVars.pop());
  }
}

void Invariant::registerDefinedVariable(Engine& engine, VarId id) {
  if (_primaryDefinedVar == NULL_ID) {
    _primaryDefinedVar = id;
  } else {
    _definedVars.push_back(id);
  }
  engine.registerDefinedVariable(id, _id);
}

void Invariant::updateValue(Timestamp ts, Engine& engine, VarId id, Int val) {
  engine.updateValue(ts, id, val);
}

void Invariant::incValue(Timestamp ts, Engine& engine, VarId id, Int val) {
  engine.incValue(ts, id, val);
}
#include "invariants/elementVar.hpp"

ElementVar::ElementVar(VarId i, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), _i(i), _X(std::move(X)), _b(b) {
  _modifiedVars.reserve(1);
}

void ElementVar::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(_id != NULL_ID);

  registerDefinedVariable(engine, _b);
  engine.registerInvariantDependsOnVar(_id, _i, LocalId(0));
  for (size_t i = 0; i < _X.size(); ++i) {
    engine.registerInvariantDependsOnVar(_id, _X[i], LocalId(0));
  }
}

void ElementVar::recompute(Timestamp ts, Engine& engine) {
  updateValue(
      ts, engine, _b,
      engine.getValue(
          ts, _X.at(static_cast<unsigned long>(engine.getValue(ts, _i)))));
}

void ElementVar::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementVar::getNextDependency(Timestamp ts, Engine& engine) {
  _state.incValue(ts, 1);
  if (_state.getValue(ts) == 0) {
    return _i;
  } else if (_state.getValue(ts) == 1) {
    return _X.at(static_cast<unsigned long>(engine.getValue(ts, _i)));
  } else {
    return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentDependencyChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void ElementVar::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

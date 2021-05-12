#include "invariants/elementVar.hpp"

ElementVar::ElementVar(VarId i, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), _i(i), _X(std::move(X)), _b(b) {
  _modifiedVars.reserve(1);
}

void ElementVar::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(_id != NULL_ID);

  registerDefinedVariable(e, _b);
  e.registerInvariantDependsOnVar(_id, _i, LocalId(0));
  for (size_t i = 0; i < _X.size(); ++i) {
    e.registerInvariantDependsOnVar(_id, _X[i], LocalId(0));
  }
}

void ElementVar::recompute(Timestamp t, Engine& e) {
  updateValue(
      t, e, _b,
      e.getValue(t, _X.at(static_cast<unsigned long>(e.getValue(t, _i)))));
}

void ElementVar::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId ElementVar::getNextDependency(Timestamp t, Engine& e) {
  _state.incValue(t, 1);
  if (_state.getValue(t) == 0) {
    return _i;
  } else if (_state.getValue(t) == 1) {
    return _X.at(static_cast<unsigned long>(e.getValue(t, _i)));
  } else {
    return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void ElementVar::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }

#include "invariants/elementConst.hpp"

#include "core/engine.hpp"

ElementConst::ElementConst(VarId i, std::vector<Int> A, VarId b)
    : Invariant(NULL_ID), _i(i), _A(std::move(A)), _b(b) {
  _modifiedVars.reserve(1);
}

void ElementConst::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(_id != NULL_ID);

  registerDefinedVariable(e, _b);
  e.registerInvariantDependsOnVar(_id, _i, 0);
}

void ElementConst::recompute(Timestamp t, Engine& e) {
  updateValue(t, e, _b, _A.at(static_cast<unsigned long>(e.getValue(t, _i))));
}

void ElementConst::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId ElementConst::getNextDependency(Timestamp t, Engine&) {
  _state.incValue(t, 1);
  if (_state.getValue(t) == 0) {
    return _i;
  } else {
    return NULL_ID;  // Done
  }
}

void ElementConst::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void ElementConst::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }

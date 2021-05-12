#include "invariants/elementConst.hpp"

#include "core/engine.hpp"

ElementConst::ElementConst(VarId i, std::vector<Int> A, VarId b)
    : Invariant(NULL_ID), _i(i), _A(std::move(A)), _b(b) {
  _modifiedVars.reserve(1);
}

void ElementConst::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(_id != NULL_ID);

  registerDefinedVariable(engine, _b);
  engine.registerInvariantParameter(_id, _i, 0);
}

void ElementConst::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _b,
              _A.at(static_cast<unsigned long>(engine.getValue(ts, _i))));
}

void ElementConst::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementConst::getNextParameter(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  if (_state.getValue(ts) == 0) {
    return _i;
  } else {
    return NULL_ID;  // Done
  }
}

void ElementConst::notifyCurrentParameterChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void ElementConst::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

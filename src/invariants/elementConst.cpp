#include "invariants/elementConst.hpp"

#include "core/engine.hpp"

ElementConst::ElementConst(VarId index, std::vector<Int> array, VarId y)
    : Invariant(NULL_ID), _index(index), _array(std::move(array)), _y(y) {
  _modifiedVars.reserve(1);
}

void ElementConst::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(_id != NULL_ID);

  registerDefinedVariable(engine, _y);
  engine.registerInvariantInput(_id, _index, 0);
}

void ElementConst::recompute(Timestamp ts, Engine& engine) {
  updateValue(
      ts, engine, _y,
      _array.at(static_cast<unsigned long>(engine.getValue(ts, _index))));
}

void ElementConst::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementConst::getNextInput(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  if (_state.getValue(ts) == 0) {
    return _index;
  } else {
    return NULL_ID;  // Done
  }
}

void ElementConst::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void ElementConst::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

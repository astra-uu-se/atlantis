#include "invariants/elementVar.hpp"

ElementVar::ElementVar(VarId index, std::vector<VarId> varArray, VarId y)
    : Invariant(NULL_ID), _index(index), _varArray(std::move(varArray)), _y(y) {
  _modifiedVars.reserve(1);
}

void ElementVar::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(_id != NULL_ID);

  registerDefinedVariable(engine, _y);
  engine.registerInvariantParameter(_id, _index, LocalId(0));
  for (size_t index = 0; index < _varArray.size(); ++index) {
    engine.registerInvariantParameter(_id, _varArray[index], LocalId(0));
  }
}

void ElementVar::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _y,
              engine.getValue(ts, _varArray.at(static_cast<unsigned long>(
                                      engine.getValue(ts, _index)))));
}

void ElementVar::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementVar::getNextParameter(Timestamp ts, Engine& engine) {
  _state.incValue(ts, 1);
  if (_state.getValue(ts) == 0) {
    return _index;
  } else if (_state.getValue(ts) == 1) {
    return _varArray.at(
        static_cast<unsigned long>(engine.getValue(ts, _index)));
  } else {
    return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentParameterChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void ElementVar::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

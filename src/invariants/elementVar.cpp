#include "invariants/elementVar.hpp"

ElementVar::ElementVar(VarId index, std::vector<VarId> varArray, VarId y)
    : Invariant(NULL_ID), _index(index), _varArray(std::move(varArray)), _y(y) {
  _modifiedVars.reserve(1);
}

void ElementVar::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(_id != NULL_ID);

  registerDefinedVariable(engine, _y);
  engine.registerInvariantInput(_id, _index, LocalId(0));
  for (const auto index : _varArray) {
    engine.registerInvariantInput(_id, index, LocalId(0));
  }
}

void ElementVar::recompute(Timestamp ts, Engine& engine) {
  assert(0 <= engine.getValue(ts, _index) &&
         static_cast<size_t>(engine.getValue(ts, _index)) < _varArray.size());
  updateValue(
      ts, engine, _y,
      engine.getValue(
          ts, _varArray[static_cast<size_t>(engine.getValue(ts, _index))]));
}

void ElementVar::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementVar::getNextInput(Timestamp ts, Engine& engine) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _index;
    case 1:
      assert(0 <= engine.getValue(ts, _index) &&
             static_cast<size_t>(engine.getValue(ts, _index)) <
                 _varArray.size());
      return _varArray[static_cast<size_t>(engine.getValue(ts, _index))];
    default:
      return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void ElementVar::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

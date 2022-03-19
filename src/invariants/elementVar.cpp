#include "invariants/elementVar.hpp"

ElementVar::ElementVar(VarId index, std::vector<VarId> varArray, VarId y)
    : Invariant(NULL_ID), _index(index), _varArray(std::move(varArray)), _y(y) {
  _modifiedVars.reserve(1);
}

void ElementVar::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _index, LocalId(0));
  for (const auto inputId : _varArray) {
    engine.registerInvariantInput(_id, inputId, LocalId(0));
  }
  registerDefinedVariable(engine, _y);
}

void ElementVar::updateBounds(Engine& engine) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();
  for (Int i = std::max(Int(1), engine.lowerBound(_index));
       i <=
       std::min(static_cast<Int>(_varArray.size()), engine.upperBound(_index));
       ++i) {
    lb = std::min(lb, engine.lowerBound(_varArray[i - 1]));
    ub = std::max(ub, engine.upperBound(_varArray[i - 1]));
  }
  engine.updateBounds(_y, lb, ub);
}

void ElementVar::recompute(Timestamp ts, Engine& engine) {
  assert(toZeroIndex(engine.value(ts, _index)) < _varArray.size());
  updateValue(
      ts, engine, _y,
      engine.value(ts, _varArray[toZeroIndex(engine.value(ts, _index))]));
}

void ElementVar::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementVar::nextInput(Timestamp ts, Engine& engine) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _index;
    case 1: {
      assert(toZeroIndex(engine.value(ts, _index)) < _varArray.size());
      return _varArray[toZeroIndex(engine.value(ts, _index))];
    }
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

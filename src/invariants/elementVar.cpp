#include "invariants/elementVar.hpp"

ElementVar::ElementVar(VarId output, VarId index, std::vector<VarId> varArray)
    : Invariant(),
      _output(output),
      _index(index),
      _varArray(prependNullId(varArray)) {
  _modifiedVars.reserve(1);
}

void ElementVar::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _index, LocalId(0));
  for (size_t i = 1; i < _varArray.size(); ++i) {
    engine.registerInvariantInput(_id, _varArray[i], LocalId(0));
  }
  registerDefinedVariable(engine, _output);
}

void ElementVar::updateBounds(Engine& engine, bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();
  Int iLb = std::max(Int(1), engine.lowerBound(_index));
  Int iUb = std::min(static_cast<Int>(_varArray.size()) - Int(1),
                     engine.upperBound(_index));
  if (iLb > iUb) {
    iLb = 1;
    iUb = static_cast<Int>(_varArray.size()) - 1;
  }
  for (Int i = iLb; i <= iUb; ++i) {
    lb = std::min(lb, engine.lowerBound(_varArray[i]));
    ub = std::max(ub, engine.upperBound(_varArray[i]));
  }
  engine.updateBounds(_output, lb, ub, widenOnly);
}

void ElementVar::recompute(Timestamp ts, Engine& engine) {
  assert(safeIndex(engine.value(ts, _index)) < _varArray.size());
  updateValue(ts, engine, _output,
              engine.value(ts, _varArray[safeIndex(engine.value(ts, _index))]));
}

void ElementVar::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementVar::nextInput(Timestamp ts, Engine& engine) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _index;
    case 1: {
      assert(safeIndex(engine.value(ts, _index)) < _varArray.size());
      return _varArray[safeIndex(engine.value(ts, _index))];
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

#include "invariants/elementConst.hpp"

#include "core/engine.hpp"

ElementConst::ElementConst(VarId index, std::vector<Int> array, VarId y)
    : Invariant(NULL_ID), _index(index), _array(prependZero(array)), _y(y) {
  _modifiedVars.reserve(1);
}

void ElementConst::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  registerDefinedVariable(engine, _y);
  engine.registerInvariantInput(_id, _index, 0);
}

void ElementConst::updateBounds(Engine& engine, bool widenOnly) {
  const Int indexLb = std::max(Int(1), engine.lowerBound(_index));
  const Int indexUb = std::min(static_cast<Int>(_array.size()) - Int(1),
                               engine.upperBound(_index));

  if (indexLb > indexUb) {
    return;
  }

  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (Int i = indexLb; i <= indexUb; ++i) {
    lb = std::min(lb, _array[i]);
    ub = std::max(ub, _array[i]);
  }

  engine.updateBounds(_y, lb, ub, widenOnly);
}

void ElementConst::recompute(Timestamp ts, Engine& engine) {
  assert(safeIndex(engine.value(ts, _index)) < _array.size());
  updateValue(ts, engine, _y, _array[safeIndex(engine.value(ts, _index))]);
}

void ElementConst::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementConst::nextInput(Timestamp ts, Engine&) {
  if (_state.incValue(ts, 1) == 0) {
    return _index;
  }
  return NULL_ID;  // Done
}

void ElementConst::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void ElementConst::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

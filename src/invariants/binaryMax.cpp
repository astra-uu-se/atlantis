#include "invariants/binaryMax.hpp"

#include "core/engine.hpp"

BinaryMax::BinaryMax(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BinaryMax::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(engine, _output);
}

void BinaryMax::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(
      _output, std::max(engine.lowerBound(_x), engine.lowerBound(_y)),
      std::max(engine.upperBound(_x), engine.upperBound(_y)), widenOnly);
}

void BinaryMax::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _output,
              std::max(engine.value(ts, _x), engine.value(ts, _y)));
}

VarId BinaryMax::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BinaryMax::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void BinaryMax::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void BinaryMax::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

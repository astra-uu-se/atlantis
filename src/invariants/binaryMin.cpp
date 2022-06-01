#include "invariants/binaryMin.hpp"

#include "core/engine.hpp"

BinaryMin::BinaryMin(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BinaryMin::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(engine, _output);
}

void BinaryMin::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(
      _output, std::min(engine.lowerBound(_x), engine.lowerBound(_y)),
      std::min(engine.upperBound(_x), engine.upperBound(_y)), widenOnly);
}

void BinaryMin::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _output,
              std::min(engine.value(ts, _x), engine.value(ts, _y)));
}

VarId BinaryMin::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BinaryMin::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void BinaryMin::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void BinaryMin::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

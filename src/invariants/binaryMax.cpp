#include "invariants/binaryMax.hpp"

BinaryMax::BinaryMax(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BinaryMax::registerVars() {
  assert(!_id.equals(NULL_ID));
  _engine.registerInvariantInput(_id, _x, 0);
  _engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(_output);
}

void BinaryMax::updateBounds(bool widenOnly) {
  _engine.updateBounds(
      _output, std::max(_engine.lowerBound(_x), _engine.lowerBound(_y)),
      std::max(_engine.upperBound(_x), _engine.upperBound(_y)), widenOnly);
}

void BinaryMax::recompute(Timestamp ts) {
  updateValue(ts, _output,
              std::max(_engine.value(ts, _x), _engine.value(ts, _y)));
}

VarId BinaryMax::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BinaryMax::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void BinaryMax::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

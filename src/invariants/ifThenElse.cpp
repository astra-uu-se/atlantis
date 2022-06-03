#include "invariants/ifThenElse.hpp"

IfThenElse::IfThenElse(VarId output, VarId b, VarId x, VarId y)
    : Invariant(), _output(output), _b(b), _xy({x, y}) {
  _modifiedVars.reserve(1);
}

void IfThenElse::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _b, 0);
  engine.registerInvariantInput(_id, _xy[0], 0, true);
  engine.registerInvariantInput(_id, _xy[1], 0, true);
  registerDefinedVariable(engine, _output);
}

void IfThenElse::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(
      _output, std::min(engine.lowerBound(_xy[0]), engine.lowerBound(_xy[1])),
      std::max(engine.upperBound(_xy[0]), engine.upperBound(_xy[1])),
      widenOnly);
}

void IfThenElse::recompute(Timestamp ts, Engine& engine) {
  updateValue(
      ts, engine, _output,
      engine.value(
          ts, _dynamicInputVar.set(
                  ts, _xy[static_cast<size_t>(engine.value(ts, _b) != 0)])));
}

void IfThenElse::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId IfThenElse::nextInput(Timestamp ts, Engine& engine) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _b;
    case 1:
      return _xy[1 - (engine.value(ts, _b) == 0)];
    default:
      return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void IfThenElse::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  _dynamicInputVar.commitIf(ts);
}
#include "propagation/invariants/ifThenElse.hpp"

namespace atlantis::propagation {

IfThenElse::IfThenElse(Engine& engine, VarId output, VarId b, VarId x, VarId y)
    : Invariant(engine), _output(output), _b(b), _xy({x, y}) {
  _modifiedVars.reserve(1);
}

void IfThenElse::registerVars() {
  assert(!_id.equals(NULL_ID));
  _engine.registerInvariantInput(_id, _b, 0);
  _engine.registerInvariantInput(_id, _xy[0], 0, true);
  _engine.registerInvariantInput(_id, _xy[1], 0, true);
  registerDefinedVariable(_output);
}

VarId IfThenElse::dynamicInputVar(Timestamp ts) const noexcept {
  return _engine.value(ts,
                       _xy[static_cast<size_t>(_engine.value(ts, _b) != 0)]);
}

void IfThenElse::updateBounds(bool widenOnly) {
  _engine.updateBounds(
      _output, std::min(_engine.lowerBound(_xy[0]), _engine.lowerBound(_xy[1])),
      std::max(_engine.upperBound(_xy[0]), _engine.upperBound(_xy[1])),
      widenOnly);
}

void IfThenElse::recompute(Timestamp ts) {
  updateValue(
      ts, _output,
      _engine.value(ts, _xy[static_cast<size_t>(_engine.value(ts, _b) != 0)]));
}

void IfThenElse::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId IfThenElse::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _b;
    case 1:
      return _xy[1 - (_engine.value(ts, _b) == 0)];
    default:
      return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
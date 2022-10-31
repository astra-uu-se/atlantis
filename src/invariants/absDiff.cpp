#include "invariants/absDiff.hpp"

#include "core/engine.hpp"

AbsDiff::AbsDiff(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void AbsDiff::registerVars() {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(_output);
  _engine.registerInvariantInput(_id, _x, 0);
  _engine.registerInvariantInput(_id, _y, 0);
}

void AbsDiff::updateBounds(bool widenOnly) {
  const Int xLb = _engine.lowerBound(_x);
  const Int xUb = _engine.upperBound(_x);
  const Int yLb = _engine.lowerBound(_y);
  const Int yUb = _engine.upperBound(_y);

  const Int lb = xLb <= yUb && yLb <= xUb
                     ? 0
                     : std::min(std::abs(xLb - yUb), std::abs(yLb - xUb));

  const Int ub = std::max(std::max(std::abs(xLb - yLb), std::abs(xLb - yUb)),
                          std::max(std::abs(xUb - yLb), std::abs(xUb - yUb)));

  _engine.updateBounds(_output, lb, ub, widenOnly);
}

void AbsDiff::recompute(Timestamp ts) {
  updateValue(ts, _output,
              std::abs(_engine.value(ts, _x) - _engine.value(ts, _y)));
}

void AbsDiff::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId AbsDiff::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void AbsDiff::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void AbsDiff::commit(Timestamp ts) { Invariant::commit(ts); }

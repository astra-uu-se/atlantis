#include "invariants/absDiff.hpp"

#include "core/engine.hpp"

AbsDiff::AbsDiff(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void AbsDiff::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _output);
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
}

void AbsDiff::updateBounds(Engine& engine, bool widenOnly) {
  const Int xLb = engine.lowerBound(_x);
  const Int xUb = engine.upperBound(_x);
  const Int yLb = engine.lowerBound(_y);
  const Int yUb = engine.upperBound(_y);

  const Int lb = xLb <= yUb && yLb <= xUb
                     ? 0
                     : std::min(std::abs(xLb - yUb), std::abs(yLb - xUb));

  const Int ub = std::max(std::max(std::abs(xLb - yLb), std::abs(xLb - yUb)),
                          std::max(std::abs(xUb - yLb), std::abs(xUb - yUb)));

  engine.updateBounds(_output, lb, ub, widenOnly);
}

void AbsDiff::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _output,
              std::abs(engine.value(ts, _x) - engine.value(ts, _y)));
}

void AbsDiff::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId AbsDiff::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void AbsDiff::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void AbsDiff::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

#include "invariants/times.hpp"

#include "core/engine.hpp"

void Times::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _a, 0);
  engine.registerInvariantInput(_id, _b, 0);
  registerDefinedVariable(engine, _y);
}

void Times::updateBounds(Engine& engine) {
  const Int aLb = engine.lowerBound(_a);
  const Int aUb = engine.upperBound(_a);
  const Int bLb = engine.lowerBound(_b);
  const Int bUb = engine.upperBound(_b);
  const std::array<const Int, 4> vals{aLb * bLb, aLb * bUb, aUb * bLb,
                                      aUb * bUb};
  const auto [lb, ub] = std::minmax_element(vals.begin(), vals.end());
  engine.updateBounds(_y, *lb, *ub);
}

void Times::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _y, engine.value(ts, _a) * engine.value(ts, _b));
}

VarId Times::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _a;
    case 1:
      return _b;
    default:
      return NULL_ID;
  }
}

void Times::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Times::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void Times::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

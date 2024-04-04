#include "atlantis/propagation/invariants/boolXor.hpp"

#include <algorithm>
#include <functional>

#include "atlantis/propagation/variables/intVar.hpp"

namespace atlantis::propagation {

/**
 * invariant output = ((x == 0) != (y == 0))
 * output does not violate if exactly one of x or y violates.
 * @param violationId id for the violationCount
 * @param x first violation variable
 * @param y second violation variable
 * @param output result
 */
BoolXor::BoolXor(SolverBase& solver, VarId output, VarViewId x, VarViewId y)
    : Invariant(solver), _output(output), _x(x), _y(y) {}

BoolXor::BoolXor(SolverBase& solver, VarViewId output, VarViewId x, VarViewId y)
    : BoolXor(solver, VarId(output), x, y) {
  assert(output.isVar());
}

void BoolXor::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, false);
  _solver.registerInvariantInput(_id, _y, false);
  registerDefinedVar(_output);
}

void BoolXor::updateBounds(bool widenOnly) {
  _solver.updateBounds(_output, 0, 1, widenOnly);
}

void BoolXor::recompute(Timestamp ts) {
  updateValue(ts, _output,
              static_cast<Int>((_solver.value(ts, _x) != 0) ==
                               (_solver.value(ts, _y) != 0)));
}

void BoolXor::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId BoolXor::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolXor::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation

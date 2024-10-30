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
  _solver.registerInvariantInput(_id, _x, LocalId(0), false);
  _solver.registerInvariantInput(_id, _y, LocalId(0), false);
  registerDefinedVar(_output);
}

void BoolXor::updateBounds(bool widenOnly) {
  Int lb = 0;
  Int ub = 1;
  const bool xIsZero = _solver.upperBound(_x) == 0;
  const bool xIsOne = _solver.lowerBound(_x) > 0;
  const bool yIsZero = _solver.upperBound(_y) == 0;
  const bool yIsOne = _solver.lowerBound(_y) > 0;
  if ((xIsZero || xIsOne) && (yIsZero || yIsOne)) {
    if (xIsZero == yIsZero || xIsOne == yIsOne) {
      lb = 1;
    } else {
      ub = 0;
    }
  }
  _solver.updateBounds(_output, lb, ub, widenOnly);
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

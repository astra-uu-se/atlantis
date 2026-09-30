#include "atlantis/propagation/views/greaterEqualConst.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

static Int compute(const Int var, const Int val) {
  return std::max<Int>(0, val - var);
}

GreaterEqualConst::GreaterEqualConst(SolverBase& solver,
                                     const VarViewId parentId, const Int val)
    : IntView(solver, parentId), _val(val) {}

Int GreaterEqualConst::value(const Timestamp ts) {
  return compute(_solver.value(ts, _parentId), _val);
}

Int GreaterEqualConst::committedValue() {
  return compute(_solver.committedValue(_parentId), _val);
}

Int GreaterEqualConst::lowerBound() const {
  return compute(_solver.upperBound(_parentId), _val);
}

Int GreaterEqualConst::upperBound() const {
  return compute(_solver.lowerBound(_parentId), _val);
}

}  // namespace atlantis::propagation

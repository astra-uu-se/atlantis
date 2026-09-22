#include "atlantis/propagation/views/lessEqualConst.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

static Int compute(const Int var, const Int val) { return std::max<Int>(0, var - val); }
LessEqualConst::LessEqualConst(SolverBase& solver, const VarViewId parentId, const Int val)
    : IntView(solver, parentId), _val(val) {}
Int LessEqualConst::value(const Timestamp ts) {
  return compute(_solver.value(ts, _parentId), _val);
}

Int LessEqualConst::committedValue() {
  return compute(_solver.committedValue(_parentId), _val);
}

Int LessEqualConst::lowerBound() const {
  return compute(_solver.lowerBound(_parentId), _val);
}

Int LessEqualConst::upperBound() const {
  return compute(_solver.upperBound(_parentId), _val);
}

}  // namespace atlantis::propagation

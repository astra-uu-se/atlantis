#include "views/equalConst.hpp"

inline static Int compute(Int var, Int val) { return std::abs(var - val); }

Int EqualConst::value(Timestamp ts) {
  return compute(_engine.value(ts, _parentId), _val);
}

Int EqualConst::committedValue() {
  return compute(_engine.committedValue(_parentId), _val);
}

Int EqualConst::lowerBound() const {
  const Int lb = _engine.lowerBound(_parentId);
  const Int ub = _engine.upperBound(_parentId);
  if (lb <= _val && _val <= ub) {
    return Int(0);
  }
  return std::min(compute(lb, _val), compute(ub, _val));
}

Int EqualConst::upperBound() const {
  return std::max(compute(_engine.lowerBound(_parentId), _val),
                  compute(_engine.upperBound(_parentId), _val));
}

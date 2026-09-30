#include "atlantis/propagation/variables/intVar.hpp"

#include <cassert>
#include <iosfwd>
#include <stdexcept>

namespace atlantis::propagation {

IntVar::IntVar(const Int lowerBound, const Int upperBound)
    : IntVar(NULL_ID, lowerBound, upperBound) {}

IntVar::IntVar(const VarId id, const Int lowerBound, const Int upperBound)
    : IntVar(id, 0, lowerBound, upperBound) {}

IntVar::IntVar(const VarId id, const Int initValue, const Int lowerBound,
               const Int upperBound)
    : IntVar(NULL_TIMESTAMP, id, initValue, lowerBound, upperBound) {}

IntVar::IntVar(const Timestamp ts, const VarId id, const Int initValue,
               const Int lowerBound, const Int upperBound)
    // todo: We need both a timestamp-zero (when
    // initialisation happens) but also a dummy timestamp.
    : Var(id),
      _value(ts, initValue),
      _lowerBound(lowerBound),
      _upperBound(upperBound) {
  if (lowerBound > upperBound) {
    throw std::out_of_range(
        "Lower bound must be smaller than or equal to upper bound");
  }
  if (initValue < lowerBound || upperBound < initValue) {
    throw std::out_of_range("value must be inside bounds");
  }
}

void IntVar::updateBounds(const Int lowerBound, const Int upperBound,
                          const bool widenOnly) {
  _lowerBound = widenOnly ? std::min(_lowerBound, lowerBound) : lowerBound;
  _upperBound = widenOnly ? std::max(_upperBound, upperBound) : upperBound;
  if (_lowerBound > _upperBound) {
    throw std::out_of_range(
        "Lower bound must be smaller than or equal to upper bound");
  }
}

std::ostream& operator<<(std::ostream& out, IntVar const& var) {
  out << "IntVar(id: " << var._id;
  out << ",ts: " << var._value.tmpTimestamp();
  out << ",c: " << var._value.committedValue();
  out << ",v: " << var._value.value(var._value.tmpTimestamp());
  out << ")";
  return out;
}
}  // namespace atlantis::propagation

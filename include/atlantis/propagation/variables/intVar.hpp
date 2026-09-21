#pragma once

#include <ostream>

#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/propagation/variables/var.hpp"

namespace atlantis::propagation {

class SolverBase;  // Forward declaration

class IntVar : public Var {
  CommittableInt _value;
  Int _lowerBound;
  Int _upperBound;

  void setValue(Timestamp timestamp, Int value);
  void incValue(Timestamp timestamp, Int inc);

  void commit();
  void commitValue(Int value);
  void commitIf(Timestamp timestamp);

  friend class SolverBase;
  friend class Solver;

 public:
  explicit IntVar(Int lowerBound, Int upperBound);
  explicit IntVar(VarId id, Int lowerBound, Int upperBound);
  explicit IntVar(VarId id, Int initValue, Int lowerBound, Int upperBound);
  explicit IntVar(Timestamp ts, VarId id, Int initValue, Int lowerBound,
                  Int upperBound);

  [[nodiscard]] bool hasChanged(Timestamp ts) const;
  [[nodiscard]] Timestamp tmpTimestamp() const;
  [[nodiscard]] Int value(Timestamp ts) const;
  [[nodiscard]] Int committedValue() const;
  [[nodiscard]] Int lowerBound() const;
  [[nodiscard]] Int upperBound() const;
  [[nodiscard]] bool inDomain(Int value) const;
  void updateBounds(Int lowerBound, Int upperBound, bool widenOnly);

  friend std::ostream& operator<<(std::ostream& out, IntVar const& var);
};

[[gnu::always_inline]] inline void IntVar::setValue(const Timestamp timestamp,
                                                    const Int value) {
  _value.setValue(timestamp, value);
}

[[gnu::always_inline]] inline void IntVar::incValue(const Timestamp timestamp,
                                                    const Int inc) {
  _value.incValue(timestamp, inc);
}

[[gnu::always_inline]] inline void IntVar::commit() { _value.commit(); }

[[gnu::always_inline]] inline void IntVar::commitValue(const Int value) {
  _value.commitValue(value);
}

[[gnu::always_inline]] inline void IntVar::commitIf(const Timestamp timestamp) {
  _value.commitIf(timestamp);
}

[[gnu::always_inline]] inline bool IntVar::hasChanged(
    const Timestamp ts) const {
  return _value.hasChanged(ts);
}

[[gnu::always_inline]] inline Timestamp IntVar::tmpTimestamp() const {
  return _value.tmpTimestamp();
}

[[gnu::always_inline]] inline Int IntVar::value(const Timestamp ts) const {
  return _value.value(ts);
}

[[gnu::always_inline]] inline Int IntVar::committedValue() const {
  return _value.committedValue();
}

[[gnu::always_inline]] inline Int IntVar::lowerBound() const {
  return _lowerBound;
}

[[gnu::always_inline]] inline Int IntVar::upperBound() const {
  return _upperBound;
}

[[gnu::always_inline]] inline bool IntVar::inDomain(const Int value) const {
  return _lowerBound <= value && value <= _upperBound;
}

}  // namespace atlantis::propagation

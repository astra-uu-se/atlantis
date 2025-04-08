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

  [[gnu::always_inline]] void setValue(Timestamp timestamp, Int value) {
    _value.setValue(timestamp, value);
  }
  [[gnu::always_inline]] void incValue(Timestamp timestamp, Int inc) {
    _value.incValue(timestamp, inc);
  }

  [[gnu::always_inline]] void commit() { _value.commit(); }
  [[gnu::always_inline]] void commitValue(Int value) {
    _value.commitValue(value);
  }
  [[gnu::always_inline]] void commitIf(Timestamp timestamp) {
    _value.commitIf(timestamp);
  }

  friend class SolverBase;
  friend class Solver;

 public:
  explicit IntVar(Int lowerBound, Int upperBound);
  explicit IntVar(VarId id, Int lowerBound, Int upperBound);
  explicit IntVar(VarId id, Int initValue, Int lowerBound, Int upperBound);
  explicit IntVar(Timestamp ts, VarId id, Int initValue, Int lowerBound,
                  Int upperBound);

  [[gnu::always_inline]] [[nodiscard]] bool hasChanged(Timestamp ts) const {
    return _value.hasChanged(ts);
  }
  [[gnu::always_inline]] [[nodiscard]] Timestamp tmpTimestamp() const {
    return _value.tmpTimestamp();
  }
  [[gnu::always_inline]] [[nodiscard]] Int value(Timestamp ts) const {
    return _value.value(ts);
  }
  [[gnu::always_inline]] [[nodiscard]] Int committedValue() const {
    return _value.committedValue();
  }
  [[gnu::always_inline]] [[nodiscard]] Int lowerBound() const {
    return _lowerBound;
  }
  [[gnu::always_inline]] [[nodiscard]] Int upperBound() const {
    return _upperBound;
  }
  [[gnu::always_inline]] [[nodiscard]] bool inDomain(Int value) const {
    return _lowerBound <= value && value <= _upperBound;
  }
  void updateBounds(Int lowerBound, Int upperBound, bool widenOnly);

  friend std::ostream& operator<<(std::ostream& out, IntVar const& var);
};

}  // namespace atlantis::propagation

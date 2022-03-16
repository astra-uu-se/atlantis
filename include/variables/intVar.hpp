#pragma once

#include <iosfwd>

#include "core/types.hpp"
#include "variables/committableInt.hpp"
#include "variables/var.hpp"

class Engine;  // Forward declaration

class IntVar : public Var {
 private:
  CommittableInt _value;
  Int _lowerBound;
  Int _upperBound;

  [[gnu::always_inline]] inline void setValue(Timestamp timestamp, Int value) {
    _value.setValue(timestamp, value);
  }
  [[gnu::always_inline]] inline void incValue(Timestamp timestamp, Int inc) {
    _value.incValue(timestamp, inc);
  }

  [[gnu::always_inline]] inline void commit() { _value.commit(); }
  [[gnu::always_inline]] inline void commitValue(Int value) {
    _value.commitValue(value);
  }
  [[gnu::always_inline]] inline void commitIf(Timestamp timestamp) {
    _value.commitIf(timestamp);
  }

  friend class Engine;
  friend class PropagationEngine;

 public:
  IntVar(Int lowerBound, Int upperBound);
  IntVar(VarId id, Int lowerBound, Int upperBound);
  IntVar(VarId id, Int initValue, Int lowerBound, Int upperBound);
  IntVar(Timestamp ts, VarId id, Int initValue, Int lowerBound, Int upperBound);
  ~IntVar() = default;

  [[gnu::always_inline]] [[nodiscard]] inline bool hasChanged(
      Timestamp ts) const {
    return _value.hasChanged(ts);
  }
  [[gnu::always_inline]] [[nodiscard]] inline Timestamp tmpTimestamp() const {
    return _value.tmpTimestamp();
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int value(Timestamp ts) const {
    return _value.value(ts);
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int committedValue() const {
    return _value.committedValue();
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int lowerBound() const {
    return _lowerBound;
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int upperBound() const {
    return _upperBound;
  }
  [[gnu::always_inline]] [[nodiscard]] inline bool inDomain(Int value) const {
    return _lowerBound <= value && value <= _upperBound;
  }
  void updateDomain(Int lowerBound, Int upperBound);

  friend std::ostream& operator<<(std::ostream& out, IntVar const& var);
};

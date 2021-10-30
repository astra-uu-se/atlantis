#pragma once

#include <iosfwd>

#include "core/types.hpp"
#include "variables/savedInt.hpp"
#include "variables/var.hpp"

class Engine;  // Forward declaration

class IntVar : public Var {
 private:
  SavedInt _value;
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
  [[gnu::always_inline]] [[nodiscard]] inline Timestamp getTmpTimestamp()
      const {
    return _value.getTmpTimestamp();
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int getValue(Timestamp ts) const {
    return _value.getValue(ts);
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int getCommittedValue() const {
    return _value.getCommittedValue();
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int getLowerBound() const {
    return _lowerBound;
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int getUpperBound() const {
    return _upperBound;
  }
  [[gnu::always_inline]] [[nodiscard]] inline Int isFixed() const {
    return _lowerBound == _upperBound;
  }
  [[gnu::always_inline]] [[nodiscard]] inline bool inDomain(Int value) const {
    return _lowerBound <= value && value <= _upperBound;
  }
  void updateDomain(Int lowerBound, Int upperBound);

  friend std::ostream& operator<<(std::ostream& out, IntVar const& var);
};

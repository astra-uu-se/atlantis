#pragma once

#include "core/savedInt.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"
#include "core/var.hpp"

class Engine;  // Forward declaration

class IntVar : public Var {
 private:
  SavedInt m_value;
  Int m_lowerBound;
  Int m_upperBound;

  [[gnu::always_inline]] inline void setValue(Timestamp timestamp,
                                              Int value) {
    m_value.setValue(timestamp, value);
  }
  [[gnu::always_inline]] inline void incValue(Timestamp timestamp,
                                              Int inc) {
    m_value.incValue(timestamp, inc);
  }

  [[gnu::always_inline]] inline void commit() { m_value.commit(); }
  [[gnu::always_inline]] inline void commitValue(Int value) {
    m_value.commitValue(value);
  }
  [[gnu::always_inline]] inline void commitIf(Timestamp timestamp) {
    m_value.commitIf(timestamp);
  }

  friend class Engine;

 public:
  IntVar(Int t_lowerBound, Int t_upperBound);
  IntVar(Id t_id, Int t_lowerBound, Int t_upperBound);
  IntVar(Id t_id, Int initValue, Int t_lowerBound, Int t_upperBound);
  ~IntVar() = default;

  [[gnu::always_inline]] inline bool hasChanged(Timestamp t) const {
    return m_value.getValue(t) != m_value.getCommittedValue();
  }
  [[gnu::always_inline]] inline Timestamp getTmpTimestamp() const {
    return m_value.getTmpTimestamp();
  }
  [[gnu::always_inline]] inline Int getValue(Timestamp t) const {
    return m_value.getValue(t);
  }
  [[gnu::always_inline]] inline Int getCommittedValue() const {
    return m_value.getCommittedValue();
  }
  [[gnu::always_inline]] inline Int getLowerBound() const {
    return m_lowerBound;
  }
  [[gnu::always_inline]] inline Int getUpperBound() const {
    return m_upperBound;
  }
  [[gnu::always_inline]] inline bool inDomain(Int t_value) const {
    return m_lowerBound <= t_value && t_value <= m_upperBound;
  }
  [[gnu::always_inline]] inline void updateDomain(Int t_lowerBound, Int t_upperBound) {
    if (t_lowerBound > t_upperBound) {
      throw std::out_of_range(
        "Lower bound must be smaller than or equal to upper bound"
      );
    }
    m_lowerBound = t_lowerBound;
    m_upperBound = t_upperBound;
  }
};

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

  [[gnu::always_inline]] inline void setValue(const Timestamp& timestamp,
                                              Int value) {
    m_value.setValue(timestamp, value);
  }
  [[gnu::always_inline]] inline void incValue(const Timestamp& timestamp,
                                              Int inc) {
    m_value.incValue(timestamp, inc);
  }

  [[gnu::always_inline]] inline void commit() { m_value.commit(); }
  [[gnu::always_inline]] inline void commitValue(Int value) {
    m_value.commitValue(value);
  }
  [[gnu::always_inline]] inline void commitIf(const Timestamp& timestamp) {
    m_value.commitIf(timestamp);
  }

  friend class Engine;

 public:
  IntVar();
  IntVar(Id t_id);
  IntVar(Id t_id, Int initValue);
  IntVar(Id t_id, Int initValue, Int lowerBound, Int upperBound);
  ~IntVar() = default;

  [[gnu::always_inline]] inline bool hasChanged(const Timestamp& t) const {
    return m_value.getValue(t) != m_value.getCommittedValue();
  }
  [[gnu::always_inline]] inline const Timestamp& getTmpTimestamp() const {
    return m_value.getTmpTimestamp();
  }
  [[gnu::always_inline]] inline Int getValue(const Timestamp& t) const {
    return m_value.getValue(t);
  }
  [[gnu::always_inline]] inline Int getCommittedValue() const {
    return m_value.getCommittedValue();
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

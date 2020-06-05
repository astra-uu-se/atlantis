#pragma once

#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "core/var.hpp"
#include "core/tracer.hpp"

class Engine;  // Forward declaration

class IntVar : public Var {
 private:
  SavedInt m_value;

 public:
  IntVar();
  IntVar(Id t_id);
  ~IntVar() = default;

  [[gnu::always_inline]] inline Int getValue(const Timestamp& t) {
    return m_value.getValue(t);
  }
  [[gnu::always_inline]] inline Int getCommittedValue() {
    return m_value.getCommittedValue();
  }
  [[gnu::always_inline]] inline void setNewValue(const Timestamp& timestamp,
                                                 Int value) {
#ifdef VERBOSE_TRACE
#include <iostream>
    std::cout << "IntVar(" << m_id << ").setNewValue(" << timestamp << ","
              << value << ")"
              << "\n";
#endif
    m_value.setValue(timestamp, value);
  }
  [[gnu::always_inline]] inline void incValue(const Timestamp& timestamp,
                                              Int inc) {
#ifdef VERBOSE_TRACE
#include <iostream>
    std::cout << "IntVar(" << m_id << ").incValue(" << timestamp << "," << inc
              << ")"
              << "\n";
#endif
    m_value.incValue(timestamp, inc);
  }
  [[gnu::always_inline]] inline void commit() { m_value.commit(); }
  [[gnu::always_inline]] inline void commitValue(Int value) {
#ifdef VERBOSE_TRACE
#include <iostream>
    std::cout << "IntVar(" << m_id << ").commitValue(" << value << ")"
              << "\n";
#endif
    m_value.commitValue(value);
  }
  [[gnu::always_inline]] inline void commitIf(const Timestamp& timestamp) {
    m_value.commitIf(timestamp);
  }
};
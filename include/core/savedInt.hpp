#pragma once

#include <stdexcept>

#include "core/tracer.hpp"
#include "types.hpp"

class SavedInt {
 private:
  Timestamp m_tmpTime;
  Int m_savedValue;
  Int m_tmpValue;
  Int m_lowerBound;
  Int m_upperBound;

 public:
  SavedInt(const Timestamp& initTime, const Int& initValue)
      : SavedInt(initTime, initValue, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max()) {}
    
  SavedInt(const Timestamp& initTime, const Int& initValue, const Int& t_lowerBound, const Int& t_upperBound)
      : m_tmpTime(initTime),
        m_savedValue(initValue), 
        m_tmpValue(initValue), 
        m_lowerBound(t_lowerBound),
        m_upperBound(t_upperBound) {
    
    if (t_lowerBound > t_upperBound) {
      throw std::out_of_range(
        "Lower bound must be smaller than or equal to upper bound"
      );
    }
  }

  // inline Int getResetValue(const Timestamp& currentTime) noexcept {
  //   return currentTime == m_tmpTime ? m_tmpValue : (m_tmpValue =
  //   m_savedValue);
  // }

  [[gnu::always_inline]] inline const Timestamp& getTmpTimestamp() const {
    return m_tmpTime;
  }

  [[gnu::always_inline]] inline Int getValue(const Timestamp& currentTime) const
      noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  [[gnu::always_inline]] inline Int getCommittedValue() const noexcept {
    return m_savedValue;
  }

  [[gnu::always_inline]] inline void setValue(const Timestamp& currentTime,
                                              const Int& value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = value;
  }

  [[gnu::always_inline]] inline void incValue(const Timestamp& currentTime,
                                              const Int& inc) noexcept {
    m_tmpValue = (currentTime == m_tmpTime ? m_tmpValue : m_savedValue) + inc;
    m_tmpTime = currentTime;
#ifdef VERBOSE_TRACE
#include <iostream>
    std::cout << "SavedInt.incValue(" << currentTime << "," << inc << ")"
              << " tmpValue: " << m_tmpValue << "\n";
#endif
  }
  [[gnu::always_inline]] inline void commitValue(Int value) noexcept {
    m_savedValue = value;
  }

  [[gnu::always_inline]] inline void commit() noexcept {
    m_savedValue = m_tmpValue;
  }

  [[gnu::always_inline]] inline void commitIf(
      const Timestamp& currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
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

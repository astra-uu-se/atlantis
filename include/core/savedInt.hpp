#pragma once

#include "types.hpp"

class SavedInt {
 private:
  Int m_savedValue;
  Int m_tmpValue;
  Timestamp m_tmpTime;

 public:
  SavedInt(const Timestamp& initTime, const Int& initValue)
      : m_savedValue(initValue), m_tmpValue(initValue), m_tmpTime(initTime) {}

  // inline Int getResetValue(const Timestamp& currentTime) noexcept {
  //   return currentTime == m_tmpTime ? m_tmpValue : (m_tmpValue =
  //   m_savedValue);
  // }

  [[gnu::always_inline]] inline Int getValue(const Timestamp& currentTime) noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  [[gnu::always_inline]] inline Int getCommittedValue() noexcept {
    return m_savedValue;
  }

  [[gnu::always_inline]] inline void setValue(const Timestamp& currentTime,
                       const Int& value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = value;
  }

  [[gnu::always_inline]] inline void incValue(const Timestamp& currentTime, const Int& inc) noexcept {
    if (currentTime == m_tmpTime) {
      m_tmpValue += inc;
    } else {
      m_tmpValue = m_savedValue + inc;
    }
    m_tmpTime = currentTime;
  }
  [[gnu::always_inline]] inline void commitValue(Int value) noexcept { m_savedValue = value; }

  [[gnu::always_inline]] inline void commit() noexcept { m_savedValue = m_tmpValue; }

  [[gnu::always_inline]] inline void commitIf(const Timestamp& currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};

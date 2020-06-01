#pragma once

#include "types.hpp"

class SavedInt {
 private:
  Int m_savedValue;
  Int m_tmpValue;
  Timestamp m_tmpTime;

 public:
  SavedInt(Timestamp initTime, Int initValue)
      : m_savedValue(initValue), m_tmpValue(initValue), m_tmpTime(initTime) {}

  inline Int getValue(Timestamp currentTime) noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : (m_tmpValue = m_savedValue);
  }

  inline Int peekValue(Timestamp currentTime) noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  inline void setValue(Timestamp currentTime, Int value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = value;
  }

  inline void incValue(Timestamp currentTime, Int inc) noexcept {
    m_tmpTime = currentTime;
    if (currentTime == m_tmpTime) {
      m_tmpValue += inc;
    } else {
      m_tmpValue = m_savedValue + inc;
    }
  }
  inline void commitValue(Int value) noexcept { m_savedValue = value; }

  inline void commit() noexcept { m_savedValue = m_tmpValue; }

  inline void commitIf(Timestamp currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};

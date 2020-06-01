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

  __forceinline Int getValue(Timestamp currentTime) noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : (m_tmpValue = m_savedValue);
  }

  __forceinline Int peekValue(Timestamp currentTime) noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  __forceinline void setValue(Timestamp currentTime, Int value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = m_savedValue;
  }

  __forceinline void incValue(Timestamp currentTime, Int inc,
                              bool commit) noexcept {
    m_tmpTime = currentTime;
    if (currentTime == m_tmpTime) {
      m_tmpValue += inc;
    } else {
      m_tmpValue = m_savedValue + inc;
    }
  }
  __forceinline void setValueCommit(Int value) noexcept { m_savedValue = value; }

  __forceinline void commit() noexcept { m_savedValue = m_tmpValue; }

  __forceinline void commitIf(Timestamp currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};

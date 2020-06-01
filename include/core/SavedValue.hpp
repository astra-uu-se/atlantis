#pragma once

#include "types.hpp"

template<typename T>
class SavedValue {
  private:
  T m_savedValue;
  T m_tmpValue;
  Timestamp m_tmpTime;

public:
 SavedValue(Timestamp initTime, T initValue)
     : m_savedValue(initValue), m_tmpValue(initValue), m_tmpTime(initTime) {}

 __forceinline T getValue(Timestamp currentTime) noexcept {
   return currentTime == m_tmpTime ? m_tmpValue : (m_tmpValue = m_savedValue);
  }

  __forceinline T peekValue(Timestamp currentTime) noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  __forceinline void setValue(Timestamp currentTime, T value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = m_savedValue;
  }
  __forceinline void setValueCommit(T value) noexcept { m_savedValue = value; }

  __forceinline void commit() noexcept { m_savedValue = m_tmpValue; }

  __forceinline void commitIf(Timestamp currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};

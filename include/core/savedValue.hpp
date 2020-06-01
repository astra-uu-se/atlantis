#pragma once

#include "core/types.hpp"

template<typename T>
class SavedValue {
  private:
  T m_savedValue;
  T m_tmpValue;
  Timestamp m_tmpTime;

public:
 SavedValue(Timestamp initTime, T initValue)
     : m_savedValue(initValue), m_tmpValue(initValue), m_tmpTime(initTime) {}

 inline T getValue(Timestamp currentTime) noexcept {
   return currentTime == m_tmpTime ? m_tmpValue : (m_tmpValue = m_savedValue);
  }

  inline T peekValue(Timestamp currentTime) noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  inline void setValue(Timestamp currentTime, T value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = value;
  }
  inline void setValueCommit(T value) noexcept { m_savedValue = value; }

  inline void commit() noexcept { m_savedValue = m_tmpValue; }

  inline void commitIf(Timestamp currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};

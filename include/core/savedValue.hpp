#pragma once

#include <stdexcept>

#include "core/tracer.hpp"
#include "types.hpp"

template <class T>
class Saved {
 private:
  Timestamp m_tmpTime;
  T m_savedValue;
  T m_tmpValue;

 public:
  Saved(Timestamp initTime, T initValue)
      : m_tmpTime(initTime), m_savedValue(initValue), m_tmpValue(initValue) {}

  [[gnu::always_inline]] [[nodiscard]] inline bool hasChanged(
      Timestamp t) const {
    return m_tmpTime == t && m_savedValue != m_tmpValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Timestamp getTmpTimestamp()
      const {
    return m_tmpTime;
  }

  [[gnu::always_inline]] [[nodiscard]] inline T get(
      Timestamp currentTime) const noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  [[gnu::always_inline]] inline T set(Timestamp currentTime, T value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = value;
    return m_tmpValue;
  }

  [[gnu::always_inline]] inline void init(Timestamp currentTime, T value) noexcept {
    init(currentTime, value, value);
  }

  [[gnu::always_inline]] inline void init(Timestamp currentTime, T savedValue, T tmpValue) noexcept {
    m_tmpTime = currentTime;
    m_savedValue = savedValue;
    m_tmpValue = tmpValue;
  }

  [[gnu::always_inline]] inline void commitValue(T value) noexcept {
    m_savedValue = value;
  }

  [[gnu::always_inline]] inline void commitIf(Timestamp currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};

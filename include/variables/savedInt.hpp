#pragma once

#include "core/types.hpp"

class SavedInt {
 private:
  Timestamp m_tmpTime;
  Int m_savedValue;
  Int m_tmpValue;

 public:
  SavedInt(Timestamp initTime, const Int& initValue)
      : m_tmpTime(initTime), m_savedValue(initValue), m_tmpValue(initValue) {}

  [[gnu::always_inline]] [[nodiscard]] inline bool hasChanged(
      Timestamp t) const {
    return m_tmpTime == t && m_savedValue != m_tmpValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Timestamp getTmpTimestamp()
      const {
    return m_tmpTime;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Int getValue(
      Timestamp currentTime) const noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Int getCommittedValue()
      const noexcept {
    return m_savedValue;
  }

  [[gnu::always_inline]] inline Int setValue(Timestamp currentTime,
                                             Int value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = value;
    return m_tmpValue;
  }

  [[gnu::always_inline]] inline Int incValue(Timestamp currentTime,
                                             Int inc) noexcept {
    m_tmpValue = (currentTime == m_tmpTime ? m_tmpValue : m_savedValue) + inc;
    m_tmpTime = currentTime;
    return m_tmpValue;
  }
  [[gnu::always_inline]] inline void commitValue(Int value) noexcept {
    m_savedValue = value;
    // clear what the correct value is at tmp_time.
  }

  [[gnu::always_inline]] inline void commit() noexcept {
    // todo: do we really want this? Very dangerous to just
    // commit regardless of time.
    m_savedValue = m_tmpValue;
  }

  [[gnu::always_inline]] inline void commitIf(Timestamp currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};

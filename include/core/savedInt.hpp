#pragma once

#include "core/tracer.hpp"
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
  }
  [[gnu::always_inline]] inline void commitValue(Int value) noexcept {
    m_savedValue = value;
    // m_tmpValue = value;  // If we do not update the tmp value, then it is not
                         // clear what the correct value is at tmp_time.
  }

  [[gnu::always_inline]] inline void commit() noexcept {
    // assert(false);  // todo: do we really want this? Very dangerous to just
                    // commit regardless of time.
    m_savedValue = m_tmpValue;
  }

  [[gnu::always_inline]] inline void commitIf(
      const Timestamp& currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};

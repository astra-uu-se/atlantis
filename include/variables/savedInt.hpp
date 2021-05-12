#pragma once

#include "core/types.hpp"

class SavedInt {
 private:
  Timestamp _tmpTime;
  Int _savedValue;
  Int _tmpValue;

 public:
  SavedInt(Timestamp initTime, const Int& initValue)
      : _tmpTime(initTime), _savedValue(initValue), _tmpValue(initValue) {}

  [[gnu::always_inline]] [[nodiscard]] inline bool hasChanged(
      Timestamp ts) const {
    return _tmpTime == ts && _savedValue != _tmpValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Timestamp getTmpTimestamp()
      const {
    return _tmpTime;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Int getValue(
      Timestamp currentTime) const noexcept {
    return currentTime == _tmpTime ? _tmpValue : _savedValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Int getCommittedValue()
      const noexcept {
    return _savedValue;
  }

  [[gnu::always_inline]] inline Int setValue(Timestamp currentTime,
                                             Int value) noexcept {
    _tmpTime = currentTime;
    _tmpValue = value;
    return _tmpValue;
  }

  [[gnu::always_inline]] inline Int incValue(Timestamp currentTime,
                                             Int inc) noexcept {
    _tmpValue = (currentTime == _tmpTime ? _tmpValue : _savedValue) + inc;
    _tmpTime = currentTime;
    return _tmpValue;
  }
  [[gnu::always_inline]] inline void commitValue(Int value) noexcept {
    _savedValue = value;
    // clear what the correct value is at tmp_time.
  }

  [[gnu::always_inline]] inline void commit() noexcept {
    // todo: do we really want this? Very dangerous to just
    // commit regardless of time.
    _savedValue = _tmpValue;
  }

  [[gnu::always_inline]] inline void commitIf(Timestamp currentTime) noexcept {
    if (_tmpTime == currentTime) {
      _savedValue = _tmpValue;
    }
  }
};

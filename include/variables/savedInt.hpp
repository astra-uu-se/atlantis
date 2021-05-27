#pragma once

#include "core/types.hpp"

class SavedInt {
 private:
  Timestamp _tmpTimestamp;
  Int _savedValue;
  Int _tmpValue;

 public:
  SavedInt(Timestamp initTimestamp, const Int& initValue)
      : _tmpTimestamp(initTimestamp),
        _savedValue(initValue),
        _tmpValue(initValue) {}

  [[gnu::always_inline]] [[nodiscard]] inline bool hasChanged(
      Timestamp ts) const {
    return _tmpTimestamp == ts && _savedValue != _tmpValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Timestamp tmpTimestamp() const {
    return _tmpTimestamp;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Int value(
      Timestamp currentTimestamp) const noexcept {
    return currentTimestamp == _tmpTimestamp ? _tmpValue : _savedValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Int committedValue()
      const noexcept {
    return _savedValue;
  }

  [[gnu::always_inline]] inline Int setValue(Timestamp currentTimestamp,
                                             Int value) noexcept {
    _tmpTimestamp = currentTimestamp;
    _tmpValue = value;
    return _tmpValue;
  }

  [[gnu::always_inline]] inline Int incValue(Timestamp currentTimestamp,
                                             Int inc) noexcept {
    _tmpValue =
        (currentTimestamp == _tmpTimestamp ? _tmpValue : _savedValue) + inc;
    _tmpTimestamp = currentTimestamp;
    return _tmpValue;
  }
  [[gnu::always_inline]] inline void commitValue(Int value) noexcept {
    _savedValue = value;
    // clear what the correct value is at _tmpTimestamp.
  }

  [[gnu::always_inline]] inline void commit() noexcept {
    // todo: do we really want this? Very dangerous to just
    // commit regardless of timestamp.
    _savedValue = _tmpValue;
  }

  [[gnu::always_inline]] inline void commitIf(
      Timestamp currentTimestamp) noexcept {
    if (_tmpTimestamp == currentTimestamp) {
      _savedValue = _tmpValue;
    }
  }
};

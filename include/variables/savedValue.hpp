#pragma once

#include "core/types.hpp"

template <class T>
class Saved {
 private:
  Timestamp _tmpTimestamp;
  T _savedValue;
  T _tmpValue;

 public:
  Saved(Timestamp initTimestamp, T initValue)
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

  [[gnu::always_inline]] [[nodiscard]] inline T get(
      Timestamp currentTimestamp) const noexcept {
    return currentTimestamp == _tmpTimestamp ? _tmpValue : _savedValue;
  }

  [[gnu::always_inline]] inline T set(Timestamp currentTimestamp,
                                      T value) noexcept {
    _tmpTimestamp = currentTimestamp;
    _tmpValue = value;
    return _tmpValue;
  }

  [[gnu::always_inline]] inline void init(Timestamp currentTimestamp,
                                          T value) noexcept {
    init(currentTimestamp, value, value);
  }

  [[gnu::always_inline]] inline void init(Timestamp currentTimestamp,
                                          T savedValue, T tmpValue) noexcept {
    _tmpTimestamp = currentTimestamp;
    _savedValue = savedValue;
    _tmpValue = tmpValue;
  }

  [[gnu::always_inline]] inline void commitValue(T value) noexcept {
    _savedValue = value;
  }

  [[gnu::always_inline]] inline void commitIf(
      Timestamp currentTimestamp) noexcept {
    if (_tmpTimestamp == currentTimestamp) {
      _savedValue = _tmpValue;
    }
  }
};

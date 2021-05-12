#pragma once

#include "core/types.hpp"

template <class T>
class Saved {
 private:
  Timestamp _tmpTime;
  T _savedValue;
  T _tmpValue;

 public:
  Saved(Timestamp initTime, T initValue)
      : _tmpTime(initTime), _savedValue(initValue), _tmpValue(initValue) {}

  [[gnu::always_inline]] [[nodiscard]] inline bool hasChanged(
      Timestamp t) const {
    return _tmpTime == t && _savedValue != _tmpValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Timestamp getTmpTimestamp()
      const {
    return _tmpTime;
  }

  [[gnu::always_inline]] [[nodiscard]] inline T get(
      Timestamp currentTime) const noexcept {
    return currentTime == _tmpTime ? _tmpValue : _savedValue;
  }

  [[gnu::always_inline]] inline T set(Timestamp currentTime, T value) noexcept {
    _tmpTime = currentTime;
    _tmpValue = value;
    return _tmpValue;
  }

  [[gnu::always_inline]] inline void init(Timestamp currentTime,
                                          T value) noexcept {
    init(currentTime, value, value);
  }

  [[gnu::always_inline]] inline void init(Timestamp currentTime, T savedValue,
                                          T tmpValue) noexcept {
    _tmpTime = currentTime;
    _savedValue = savedValue;
    _tmpValue = tmpValue;
  }

  [[gnu::always_inline]] inline void commitValue(T value) noexcept {
    _savedValue = value;
  }

  [[gnu::always_inline]] inline void commitIf(Timestamp currentTime) noexcept {
    if (_tmpTime == currentTime) {
      _savedValue = _tmpValue;
    }
  }
};

#pragma once

#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

template <class T>
class Committable {
 private:
  /**
   * @brief the timestamp corresponding to the new value _tmpValue
   */
  Timestamp _tmpTimestamp;
  T _committedValue;
  T _tmpValue;

 public:
  Committable(Timestamp ts, T value)
      : _tmpTimestamp(ts), _committedValue(value), _tmpValue(value) {}

  [[gnu::always_inline]] [[nodiscard]] inline bool hasChanged(
      Timestamp ts) const {
    return _tmpTimestamp == ts && _committedValue != _tmpValue;
  }

  [[gnu::always_inline]] [[nodiscard]] inline Timestamp tmpTimestamp() const {
    return _tmpTimestamp;
  }

  [[gnu::always_inline]] [[nodiscard]] inline T get(
      Timestamp currentTimestamp) const noexcept {
    return currentTimestamp == _tmpTimestamp ? _tmpValue : _committedValue;
  }

  [[gnu::always_inline]] inline T set(Timestamp ts, T newValue) noexcept {
    _tmpTimestamp = ts;
    _tmpValue = newValue;
    return _tmpValue;
  }

  [[gnu::always_inline]] inline void init(Timestamp ts, T value) noexcept {
    init(ts, value, value);
  }

  [[gnu::always_inline]] inline void init(Timestamp ts, T committedValue,
                                          T newValue) noexcept {
    _tmpTimestamp = ts;
    _committedValue = committedValue;
    _tmpValue = newValue;
  }

  [[gnu::always_inline]] inline void commitValue(T value) noexcept {
    _committedValue = value;
  }

  [[gnu::always_inline]] inline void commitIf(Timestamp ts) noexcept {
    if (_tmpTimestamp == ts) {
      _committedValue = _tmpValue;
    }
  }
};

}  // namespace atlantis::propagation

#pragma once

#include "atlantis/types.hpp"

namespace atlantis::propagation {

template <class T>
class Committable {
  /**
   * @brief the timestamp corresponding to the new value _tmpValue
   */
  Timestamp _tmpTimestamp;
  T _committedValue;
  T _tmpValue;

 public:
  Committable(Timestamp ts, T value);

  [[nodiscard]] bool hasChanged(Timestamp ts) const;

  [[nodiscard]] Timestamp tmpTimestamp() const;

  [[nodiscard]] T get(Timestamp currentTimestamp) const noexcept;

  [[nodiscard]] T committed() const noexcept;

  [[nodiscard]] T current() const noexcept;

  T set(Timestamp ts, T newValue) noexcept;

  void init(Timestamp ts, T value) noexcept;

  void init(Timestamp ts, T committedValue, T newValue) noexcept;

  void commitValue(T value) noexcept;

  void commitIf(Timestamp ts) noexcept;
};

template <class T>
Committable<T>::Committable(const Timestamp ts, T value)
    : _tmpTimestamp(ts), _committedValue(value), _tmpValue(value) {}

template <class T>
[[gnu::always_inline]] bool Committable<T>::hasChanged(
    const Timestamp ts) const {
  return _tmpTimestamp == ts && _committedValue != _tmpValue;
}

template <class T>
[[gnu::always_inline]] Timestamp Committable<T>::tmpTimestamp() const {
  return _tmpTimestamp;
}
template <class T>
[[gnu::always_inline]] T Committable<T>::get(
    const Timestamp currentTimestamp) const noexcept {
  return currentTimestamp == _tmpTimestamp ? _tmpValue : _committedValue;
}
template <class T>
[[gnu::always_inline]] T Committable<T>::committed() const noexcept {
  return _committedValue;
}
template <class T>
[[gnu::always_inline]] T Committable<T>::current() const noexcept {
  return _tmpValue;
}
template <class T>
[[gnu::always_inline]] T Committable<T>::set(const Timestamp ts,
                                             T newValue) noexcept {
  _tmpTimestamp = ts;
  _tmpValue = newValue;
  return _tmpValue;
}
template <class T>
[[gnu::always_inline]] void Committable<T>::init(const Timestamp ts,
                                                 T value) noexcept {
  init(ts, value, value);
}
template <class T>
[[gnu::always_inline]] void Committable<T>::init(const Timestamp ts,
                                                 T committedValue,
                                                 T newValue) noexcept {
  _tmpTimestamp = ts;
  _committedValue = committedValue;
  _tmpValue = newValue;
}
template <class T>
[[gnu::always_inline]] void Committable<T>::commitValue(T value) noexcept {
  _committedValue = value;
}
template <class T>
[[gnu::always_inline]] void Committable<T>::commitIf(
    const Timestamp ts) noexcept {
  if (_tmpTimestamp == ts) {
    _committedValue = _tmpValue;
  }
}

}  // namespace atlantis::propagation

#pragma once

#include "atlantis/types.hpp"

namespace atlantis::propagation {

class CommittableInt {
  /**
   * @brief the timestamp corresponding to the new value _tmpValue
   */
  Timestamp _tmpTimestamp;
  Int _committedValue;
  Int _tmpValue;

 public:
  CommittableInt(Timestamp ts, const Int& value)
      : _tmpTimestamp(ts), _committedValue(value), _tmpValue(value) {}

  CommittableInt(Timestamp ts, const Int& committedValue, const Int& tmpValue)
      : _tmpTimestamp(ts),
        _committedValue(committedValue),
        _tmpValue(tmpValue) {}

  [[gnu::always_inline]] [[nodiscard]] bool hasChanged(Timestamp ts) const {
    return _tmpTimestamp == ts && _committedValue != _tmpValue;
  }

  [[gnu::always_inline]] [[nodiscard]] Timestamp tmpTimestamp() const {
    return _tmpTimestamp;
  }

  /**
   * @brief if ts equals the timestamp of the new value, then returns
   * the new value, else returns the committed value.
   *
   * @param ts
   * @return Int
   */
  [[gnu::always_inline]] [[nodiscard]] Int value(Timestamp ts) const noexcept {
    return ts == _tmpTimestamp ? _tmpValue : _committedValue;
  }

  [[gnu::always_inline]] [[nodiscard]] Int committedValue() const noexcept {
    return _committedValue;
  }

  [[gnu::always_inline]] Int setValue(Timestamp newTimestamp,
                                      Int newValue) noexcept {
    _tmpTimestamp = newTimestamp;
    _tmpValue = newValue;
    return _tmpValue;
  }

  [[gnu::always_inline]] Int incValue(Timestamp ts, Int inc) noexcept {
    _tmpValue = (ts == _tmpTimestamp ? _tmpValue : _committedValue) + inc;
    _tmpTimestamp = ts;
    return _tmpValue;
  }
  [[gnu::always_inline]] void commitValue(Int value) noexcept {
    _committedValue = value;
  }

  [[gnu::always_inline]] void commit() noexcept {
    // todo: do we really want this? Very dangerous to just
    // commit regardless of timestamp.
    _committedValue = _tmpValue;
  }

  [[gnu::always_inline]] void commitIf(Timestamp ts) noexcept {
    if (_tmpTimestamp == ts) {
      _committedValue = _tmpValue;
    }
  }
};

}  // namespace atlantis::propagation

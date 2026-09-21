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
  CommittableInt(Timestamp ts, const Int& value);

  CommittableInt(Timestamp ts, const Int& committedValue, const Int& tmpValue);

  [[nodiscard]] bool hasChanged(Timestamp ts) const;

  [[nodiscard]] Timestamp tmpTimestamp() const;

  /**
   * @brief if ts equals the timestamp of the new value, then returns
   * the new value, else returns the committed value.
   *
   * @param ts
   * @return Int
   */
  [[nodiscard]] Int value(Timestamp ts) const noexcept;

  [[nodiscard]] Int committedValue() const noexcept;

  Int setValue(Timestamp newTimestamp,
                                      Int newValue) noexcept;

  Int incValue(Timestamp ts, Int inc) noexcept;
  void commitValue(Int value) noexcept;

  void commit() noexcept;

  void commitIf(Timestamp ts) noexcept;
};

inline CommittableInt::CommittableInt(const Timestamp ts, const Int& value)
    : _tmpTimestamp(ts), _committedValue(value), _tmpValue(value) {}
inline CommittableInt::CommittableInt(const Timestamp ts,
                                      const Int& committedValue,
                                      const Int& tmpValue)
    : _tmpTimestamp(ts), _committedValue(committedValue), _tmpValue(tmpValue) {}

[[gnu::always_inline]] inline bool CommittableInt::hasChanged(const Timestamp ts) const {
  return _tmpTimestamp == ts && _committedValue != _tmpValue;
}

[[gnu::always_inline]] inline Timestamp CommittableInt::tmpTimestamp() const { return _tmpTimestamp; }

[[gnu::always_inline]] inline Int CommittableInt::value(const Timestamp ts) const noexcept {
  return ts == _tmpTimestamp ? _tmpValue : _committedValue;
}

[[gnu::always_inline]] inline Int CommittableInt::committedValue() const noexcept {
  return _committedValue;
}

[[gnu::always_inline]] inline Int CommittableInt::setValue(const Timestamp newTimestamp,
                                    const Int newValue) noexcept {
  _tmpTimestamp = newTimestamp;
  _tmpValue = newValue;
  return _tmpValue;
}

[[gnu::always_inline]] inline Int CommittableInt::incValue(const Timestamp ts,
                                    const Int inc) noexcept {
  _tmpValue = (ts == _tmpTimestamp ? _tmpValue : _committedValue) + inc;
  _tmpTimestamp = ts;
  return _tmpValue;
}

[[gnu::always_inline]] inline void CommittableInt::commitValue(const Int value) noexcept {
  _committedValue = value;
}

[[gnu::always_inline]] inline void CommittableInt::commit() noexcept {
  // todo: do we really want this? Very dangerous to just
  // commit regardless of timestamp.
  _committedValue = _tmpValue;
}

[[gnu::always_inline]] inline void CommittableInt::commitIf(const Timestamp ts) noexcept {
  if (_tmpTimestamp == ts) {
    _committedValue = _tmpValue;
  }
}

}  // namespace atlantis::propagation

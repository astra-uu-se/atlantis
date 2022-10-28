#pragma once

#include <limits>
#include <utility>
#include <vector>

#include "constraints/constraint.hpp"
#include "core/types.hpp"

class Engine;

/**
 * output <- number of occurrences of y in variables
 *
 */

class Count : public Invariant {
 private:
  const VarId _output;
  const VarId _y;
  const std::vector<VarId> _variables;
  std::vector<CommittableInt> _localValues;
  std::vector<CommittableInt> _counts;
  Int _offset;
  void increaseCount(Timestamp ts, Int value);
  void decreaseCount(Timestamp ts, Int value);
  signed char count(Timestamp ts, Int value);

 public:
  explicit Count(Engine&, VarId output, VarId y, std::vector<VarId> varArray);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

inline void Count::increaseCount(Timestamp ts, Int value) {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return;
  }
  assert(_counts[value - _offset].value(ts) + 1 > 0);
  assert(_counts[value - _offset].value(ts) + 1 <=
         static_cast<Int>(_variables.size()));
  _counts[value - _offset].incValue(ts, 1);
}

inline void Count::decreaseCount(Timestamp ts, Int value) {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return;
  }
  assert(_counts[value - _offset].value(ts) - 1 >= 0);
  assert(_counts[value - _offset].value(ts) - 1 <
         static_cast<Int>(_variables.size()));
  _counts[value - _offset].incValue(ts, -1);
}

inline signed char Count::count(Timestamp ts, Int value) {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return 0;
  }
  assert(0 <= value - _offset &&
         static_cast<size_t>(value - _offset) <= _counts.size());
  return _counts[value - _offset].value(ts);
}

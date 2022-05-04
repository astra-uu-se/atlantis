#pragma once

#include <cassert>
#include <vector>

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class CommittableInt;  // forward declare
class Engine;

class AllDifferent : public Constraint {
 protected:
  std::vector<VarId> _variables;
  std::vector<CommittableInt> _localValues;
  std::vector<CommittableInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  AllDifferent(VarId violationId, std::vector<VarId> variables);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};

inline signed char AllDifferent::increaseCount(Timestamp ts, Int value) {
  assert(0 <= value - _offset &&
         static_cast<size_t>(value - _offset) < _counts.size());
  assert(_counts[value - _offset].value(ts) + 1 >= 0);
  assert(_counts[value - _offset].value(ts) + 1 <=
         static_cast<Int>(_variables.size()));
  return _counts[value - _offset].incValue(ts, 1) >= 2 ? 1 : 0;
}

inline signed char AllDifferent::decreaseCount(Timestamp ts, Int value) {
  assert(0 <= value - _offset &&
         static_cast<size_t>(value - _offset) < _counts.size());
  assert(_counts[value - _offset].value(ts) - 1 >= 0);
  assert(_counts[value - _offset].value(ts) - 1 <=
         static_cast<Int>(_variables.size()));
  return _counts[value - _offset].incValue(ts, -1) >= 1 ? -1 : 0;
}

#pragma once

#include <cassert>
#include <vector>

#include "core/types.hpp"
#include "variables/intVar.hpp"
// #include "variables/savedInt.hpp"
#include "constraint.hpp"

class SavedInt;  // forward declare
class Engine;

class AllDifferent : public Constraint {
 private:
  std::vector<VarId> _variables;
  std::vector<SavedInt> _localValues;
  std::vector<SavedInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  AllDifferent(VarId violationId, std::vector<VarId> variables);

#ifndef CBLS_TEST
  void init(Timestamp, Engine&) final;
  void recompute(Timestamp, Engine&) final;
  void notifyIntChanged(Timestamp, Engine&, LocalId) final;
  void commit(Timestamp, Engine&) final;
  VarId getNextInput(Timestamp, Engine&) final;
  void notifyCurrentInputChanged(Timestamp, Engine&) final;
#else
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
#endif
};

inline signed char AllDifferent::increaseCount(Timestamp ts, Int value) {
  assert(0 <= value - _offset &&
         static_cast<size_t>(value - _offset) < _counts.size());
  assert(_counts[value - _offset].getValue(ts) + 1 >= 0);
  assert(_counts[value - _offset].getValue(ts) + 1 <=
         static_cast<Int>(_variables.size()));
  return _counts[value - _offset].incValue(ts, 1) >= 2 ? 1 : 0;
}

inline signed char AllDifferent::decreaseCount(Timestamp ts, Int value) {
  assert(0 <= value - _offset &&
         static_cast<size_t>(value - _offset) < _counts.size());
  assert(_counts[value - _offset].getValue(ts) - 1 >= 0);
  assert(_counts[value - _offset].getValue(ts) - 1 <=
         static_cast<Int>(_variables.size()));
  return _counts[value - _offset].incValue(ts, -1) >= 1 ? -1 : 0;
}

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

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};

inline signed char AllDifferent::increaseCount(Timestamp ts, Int value) {
  const Int newCount = _counts.at(value - _offset).incValue(ts, 1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(_variables.size()));
  return newCount >= 2 ? 1 : 0;
}

inline signed char AllDifferent::decreaseCount(Timestamp ts, Int value) {
  const Int newCount = _counts.at(value - _offset).incValue(ts, -1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(_variables.size()));
  return newCount >= 1 ? -1 : 0;
}

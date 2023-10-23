#pragma once

#include <cassert>
#include <limits>
#include <vector>

#include "constraint.hpp"
#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/variables/committableInt.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::propagation {

class AllDifferent : public Constraint {
 protected:
  std::vector<VarId> _variables;
  std::vector<Int> _committedValues;
  std::vector<CommittableInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  explicit AllDifferent(Engine&, VarId violationId,
                        std::vector<VarId> variables);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
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

}  // namespace atlantis::propagation
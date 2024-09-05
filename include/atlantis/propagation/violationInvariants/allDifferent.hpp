#pragma once

#include <vector>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class AllDifferent : public ViolationInvariant {
 protected:
  std::vector<VarViewId> _vars;
  std::vector<CommittableInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  explicit AllDifferent(SolverBase&, VarId violationId,
                        std::vector<VarViewId>&& vars);

  explicit AllDifferent(SolverBase&, VarViewId violationId,
                        std::vector<VarViewId>&& vars);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

inline signed char AllDifferent::increaseCount(Timestamp ts, Int value) {
  if (value < _offset || static_cast<Int>(_counts.size()) <= value - _offset) {
    return 0;
  }
  assert(_counts[value - _offset].value(ts) + 1 >= 0);
  assert(_counts[value - _offset].value(ts) + 1 <=
         static_cast<Int>(_vars.size()));
  return _counts[value - _offset].incValue(ts, 1) >= 2 ? 1 : 0;
}

inline signed char AllDifferent::decreaseCount(Timestamp ts, Int value) {
  if (value < _offset || static_cast<Int>(_counts.size()) <= value - _offset) {
    return 0;
  }
  assert(_counts[value - _offset].value(ts) - 1 >= 0);
  assert(_counts[value - _offset].value(ts) - 1 <=
         static_cast<Int>(_vars.size()));
  return _counts[value - _offset].incValue(ts, -1) >= 1 ? -1 : 0;
}

}  // namespace atlantis::propagation

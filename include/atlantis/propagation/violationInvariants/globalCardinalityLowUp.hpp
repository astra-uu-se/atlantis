#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/propagation/variables/intVar.hpp"
#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class GlobalCardinalityLowUp : public ViolationInvariant {
 private:
  std::vector<VarId> _vars;
  std::vector<Int> _lowerBounds;
  std::vector<Int> _upperBounds;
  std::vector<Int> _committedValues;
  CommittableInt _shortage;
  CommittableInt _excess;
  std::vector<CommittableInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  GlobalCardinalityLowUp(SolverBase&, VarId violationId,
                         std::vector<VarId>&& vars,
                         const std::vector<Int>& cover,
                         const std::vector<Int>& bounds);

  GlobalCardinalityLowUp(SolverBase&, VarId violationId,
                         std::vector<VarId>&& vars,
                         const std::vector<Int>& cover,
                         const std::vector<Int>& lowerBounds,
                         const std::vector<Int>& upperBounds);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

inline signed char GlobalCardinalityLowUp::increaseCount(Timestamp ts,
                                                         Int value) {
  size_t pos = static_cast<size_t>(std::max<Int>(
      0, std::min(Int(_lowerBounds.size()) - 1, value - _offset)));
  if (_lowerBounds.at(pos) < 0) {
    return 0;
  }
  Int newCount = _counts.at(pos).incValue(ts, 1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(_vars.size()));
  return newCount > _upperBounds.at(pos)
             ? 1
             : (newCount > _lowerBounds.at(pos) ? 0 : -1);
}

inline signed char GlobalCardinalityLowUp::decreaseCount(Timestamp ts,
                                                         Int value) {
  size_t pos = static_cast<size_t>(std::max<Int>(
      0, std::min(Int(_lowerBounds.size()) - 1, value - _offset)));
  if (_lowerBounds.at(pos) < 0) {
    return 0;
  }

  Int newCount = _counts.at(pos).incValue(ts, -1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(_vars.size()));
  return newCount < _lowerBounds.at(pos)
             ? 1
             : (newCount < _upperBounds.at(pos) ? 0 : -1);
}

}  // namespace atlantis::propagation

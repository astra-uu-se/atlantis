#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "propagation/violationInvariants/violationInvariant.hpp"
#include "propagation/solver.hpp"
#include "propagation/variables/committableInt.hpp"
#include "propagation/variables/intVar.hpp"
#include "types.hpp"

namespace atlantis::propagation {

template <bool IsClosed>
class GlobalCardinalityConst : public ViolationInvariant {
 private:
  const std::vector<VarId> _vars;
  std::vector<Int> _lowerBound;
  std::vector<Int> _upperBound;
  std::vector<Int> _committedValues;
  CommittableInt _shortage;
  CommittableInt _excess;
  std::vector<CommittableInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  GlobalCardinalityConst(SolverBase&, VarId violationId,
                         std::vector<VarId> vars, const std::vector<Int>& cover,
                         const std::vector<Int>& counts);
  GlobalCardinalityConst(SolverBase&, VarId violationId,
                         std::vector<VarId> vars, const std::vector<Int>& cover,
                         const std::vector<Int>& lowerBound,
                         const std::vector<Int>& upperBound);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

template <bool IsClosed>
inline signed char GlobalCardinalityConst<IsClosed>::increaseCount(Timestamp ts,
                                                                   Int value) {
  size_t pos = static_cast<size_t>(
      std::max<Int>(0, std::min(Int(_lowerBound.size()) - 1, value - _offset)));
  if constexpr (!IsClosed) {
    if (_lowerBound.at(pos) < 0) {
      return 0;
    }
  }
  Int newCount = _counts.at(pos).incValue(ts, 1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(_vars.size()));
  return newCount > _upperBound.at(pos)
             ? 1
             : (newCount > _lowerBound.at(pos) ? 0 : -1);
}

template <bool IsClosed>
inline signed char GlobalCardinalityConst<IsClosed>::decreaseCount(Timestamp ts,
                                                                   Int value) {
  size_t pos = static_cast<size_t>(
      std::max<Int>(0, std::min(Int(_lowerBound.size()) - 1, value - _offset)));
  if constexpr (!IsClosed) {
    if (_lowerBound.at(pos) < 0) {
      return 0;
    }
  }
  Int newCount = _counts.at(pos).incValue(ts, -1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(_vars.size()));
  return newCount < _lowerBound.at(pos)
             ? 1
             : (newCount < _upperBound.at(pos) ? 0 : -1);
}

}  // namespace atlantis::propagation
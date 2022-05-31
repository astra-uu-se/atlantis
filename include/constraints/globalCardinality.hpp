#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class CommittableInt;  // forward declare
class Engine;

template <bool IsClosed>
class GlobalCardinality : public Constraint {
 private:
  const std::vector<VarId> _variables;
  std::vector<Int> _lowerBound, _upperBound;
  std::vector<CommittableInt> _localValues;
  CommittableInt _shortage, _excess;
  std::vector<CommittableInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  explicit GlobalCardinality(VarId violationId, std::vector<VarId> variables,
                             const std::vector<Int>& cover,
                             const std::vector<Int>& counts);
  explicit GlobalCardinality(VarId violationId, std::vector<VarId> variables,
                             const std::vector<Int>& cover,
                             const std::vector<Int>& lowerBound,
                             const std::vector<Int>& upperBound);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine& e) override;
  void notifyCurrentInputChanged(Timestamp, Engine& e) override;
};

template <bool IsClosed>
inline signed char GlobalCardinality<IsClosed>::increaseCount(Timestamp ts,
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
  assert(newCount <= static_cast<Int>(_variables.size()));
  return newCount > _upperBound.at(pos)
             ? 1
             : (newCount > _lowerBound.at(pos) ? 0 : -1);
}

template <bool IsClosed>
inline signed char GlobalCardinality<IsClosed>::decreaseCount(Timestamp ts,
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
  assert(newCount <= static_cast<Int>(_variables.size()));
  return newCount < _lowerBound.at(pos)
             ? 1
             : (newCount < _upperBound.at(pos) ? 0 : -1);
}
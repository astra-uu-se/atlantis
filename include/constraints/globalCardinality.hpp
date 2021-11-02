#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "core/types.hpp"
#include "variables/intVar.hpp"
// #include "variables/savedInt.hpp"
#include "constraint.hpp"

class SavedInt;  // forward declare
class Engine;

template <bool IsClosed>
class GlobalCardinality : public Constraint {
 private:
  std::vector<VarId> _variables;
  std::vector<Int> _cover;
  std::vector<Int> _lowerBound;
  std::vector<Int> _upperBound;
  std::vector<SavedInt> _localValues;
  SavedInt _excess;
  SavedInt _shortage;
  std::vector<SavedInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  GlobalCardinality(VarId violationId, std::vector<VarId> t_variables,
                    std::vector<Int> cover, std::vector<Int> t_counts);
  GlobalCardinality(VarId violationId, std::vector<VarId> t_variables,
                    std::vector<Int> cover, std::vector<Int> lowerBound,
                    std::vector<Int> upperBound);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine& e) override;
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
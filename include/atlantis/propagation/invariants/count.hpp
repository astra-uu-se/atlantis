#pragma once

#include <cassert>
#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * output <- number of occurrences of needle in variables
 *
 */

class Count : public Invariant {
 private:
  VarId _output;
  VarViewId _needle;
  std::vector<VarViewId> _vars;
  std::vector<CommittableInt> _counts;
  Int _offset;
  void increaseCount(Timestamp ts, Int value);
  void decreaseCount(Timestamp ts, Int value);
  signed char count(Timestamp ts, Int value);

 public:
  explicit Count(SolverBase&, VarId output, VarViewId needle,
                 std::vector<VarViewId>&& varArray);

  explicit Count(SolverBase&, VarViewId output, VarViewId needle,
                 std::vector<VarViewId>&& varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

inline void Count::increaseCount(Timestamp ts, Int value) {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return;
  }
  assert(_counts[value - _offset].value(ts) + 1 > 0);
  assert(_counts[value - _offset].value(ts) + 1 <=
         static_cast<Int>(_vars.size()));
  _counts[value - _offset].incValue(ts, 1);
}

inline void Count::decreaseCount(Timestamp ts, Int value) {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return;
  }
  assert(_counts[value - _offset].value(ts) - 1 >= 0);
  assert(_counts[value - _offset].value(ts) - 1 <
         static_cast<Int>(_vars.size()));
  _counts[value - _offset].incValue(ts, -1);
}

inline signed char Count::count(Timestamp ts, Int value) {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return 0;
  }
  assert(0 <= value - _offset &&
         static_cast<size_t>(value - _offset) <= _counts.size());
  return static_cast<signed char>(_counts[value - _offset].value(ts));
}

}  // namespace atlantis::propagation

#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "propagation/violationInvariants/violationInvariant.hpp"
#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/variables/committableInt.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::propagation {

class GlobalCardinalityClosed : public ViolationInvariant {
 private:
  const std::vector<VarId> _outputs;
  const std::vector<VarId> _inputs;
  const std::vector<Int> _cover;
  std::vector<Int> _coverVarIndex;
  std::vector<Int> _committedValues;
  std::vector<CommittableInt> _counts;
  Int _offset;
  Int increaseCount(Timestamp ts, Int value);
  Int decreaseCount(Timestamp ts, Int value);
  void updateOutput(Timestamp ts, Int value);

 public:
  GlobalCardinalityClosed(SolverBase&, VarId violationId,
                          std::vector<VarId> outputs, std::vector<VarId> inputs,
                          std::vector<Int> cover);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

inline Int GlobalCardinalityClosed::increaseCount(Timestamp ts, Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    _counts[_coverVarIndex[value - _offset]].incValue(ts, 1);
    return 0;
  }
  return 1;
}

inline Int GlobalCardinalityClosed::decreaseCount(Timestamp ts, Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    _counts[_coverVarIndex[value - _offset]].incValue(ts, -1);
    return 0;
  }
  return 1;
}

inline void GlobalCardinalityClosed::updateOutput(Timestamp ts, Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    updateValue(ts, _outputs[_coverVarIndex[value - _offset]],
                _counts[_coverVarIndex[value - _offset]].value(ts));
  }
}

}  // namespace atlantis::propagation
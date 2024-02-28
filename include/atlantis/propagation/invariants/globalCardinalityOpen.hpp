#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "invariant.hpp"
#include "propagation/solver.hpp"
#include "propagation/variables/committableInt.hpp"
#include "propagation/variables/intVar.hpp"
#include "types.hpp"

namespace atlantis::propagation {

class GlobalCardinalityOpen : public Invariant {
 private:
  std::vector<VarId> _outputs;
  std::vector<VarId> _inputs;
  std::vector<Int> _cover;
  std::vector<Int> _coverVarIndex;
  std::vector<Int> _committedValues;
  std::vector<CommittableInt> _counts;
  Int _offset;
  void increaseCount(Timestamp ts, Int value);
  void decreaseCountAndUpdateOutput(Timestamp ts, Int value);
  void increaseCountAndUpdateOutput(Timestamp ts, Int value);

 public:
  GlobalCardinalityOpen(SolverBase&, std::vector<VarId>&& outputs,
                        std::vector<VarId>&& inputs, std::vector<Int>&& cover);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

inline void GlobalCardinalityOpen::increaseCount(Timestamp ts, Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    _counts[_coverVarIndex[value - _offset]].incValue(ts, 1);
  }
}

inline void GlobalCardinalityOpen::decreaseCountAndUpdateOutput(Timestamp ts,
                                                                Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    updateValue(ts, _outputs[_coverVarIndex[value - _offset]],
                _counts[_coverVarIndex[value - _offset]].incValue(ts, -1));
  }
}

inline void GlobalCardinalityOpen::increaseCountAndUpdateOutput(Timestamp ts,
                                                                Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    updateValue(ts, _outputs[_coverVarIndex[value - _offset]],
                _counts[_coverVarIndex[value - _offset]].incValue(ts, 1));
  }
}

}  // namespace atlantis::propagation
#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariant.hpp"
#include "variables/committableInt.hpp"
#include "variables/intVar.hpp"

class CommittableInt;  // forward declare
class Engine;

class GlobalCardinalityOpen : public Invariant {
 private:
  const std::vector<VarId> _outputs;
  const std::vector<VarId> _inputs;
  const std::vector<Int> _cover;
  std::vector<Int> _coverVarIndex;
  std::vector<Int> _committedValues;
  std::vector<CommittableInt> _counts;
  Int _offset;
  void increaseCount(Timestamp ts, Int value);
  void decreaseCountAndUpdateOutput(Timestamp ts, Int value);
  void increaseCountAndUpdateOutput(Timestamp ts, Int value);

 public:
  GlobalCardinalityOpen(Engine&, std::vector<VarId> outputs,
                        std::vector<VarId> inputs, std::vector<Int> cover);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
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
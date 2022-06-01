#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class CommittableInt;  // forward declare
class Engine;

class GlobalCardinalityClosed : public Constraint {
 private:
  const std::vector<VarId> _outputs;
  const std::vector<VarId> _inputs;
  const std::vector<Int> _cover;
  std::vector<Int> _coverVarIndex;
  std::vector<CommittableInt> _localValues;
  std::vector<CommittableInt> _counts;
  Int _offset;
  Int increaseCount(Timestamp ts, Int value);
  Int decreaseCount(Timestamp ts, Int value);
  void updateOutput(Timestamp ts, Engine& engine, Int value);

 public:
  GlobalCardinalityClosed(VarId violationId, std::vector<VarId> outputs,
                          std::vector<VarId> inputs, std::vector<Int> cover);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine& e) override;
  void notifyCurrentInputChanged(Timestamp, Engine& e) override;
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

inline void GlobalCardinalityClosed::updateOutput(Timestamp ts, Engine& engine,
                                                  Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    updateValue(ts, engine, _outputs[_coverVarIndex[value - _offset]],
                _counts[_coverVarIndex[value - _offset]].value(ts));
  }
}
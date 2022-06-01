#pragma once

#include <limits>
#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"
#include "utils/priorityList.hpp"

/**
 * Invariant for y <- for all b == 0 in varArray
 *
 */

class ForAll : public Invariant {
 private:
  const VarId _output;
  const std::vector<VarId> _varArray;

  PriorityList _localPriority;

 public:
  explicit ForAll(VarId output, std::vector<VarId> varArray);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};

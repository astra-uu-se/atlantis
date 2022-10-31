#pragma once

#include <limits>
#include <utility>
#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"
#include "utils/priorityList.hpp"

/**
 * Invariant for output <- exists b == 0 in varArray
 *
 */

class Exists : public Invariant {
 private:
  const VarId _output;
  const std::vector<VarId> _varArray;

  PriorityList _localPriority;

 public:
  explicit Exists(Engine&, VarId output, std::vector<VarId> varArray);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

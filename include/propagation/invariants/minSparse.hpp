#pragma once

#include <limits>
#include <utility>
#include <vector>

#include "propagation/engine.hpp"
#include "propagation/invariants/invariant.hpp"
#include "propagation/utils/priorityList.hpp"
#include "types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- min(varArray)
 *
 */

class MinSparse : public Invariant {
 private:
  const VarId _output;
  const std::vector<VarId> _varArray;

  PriorityList _localPriority;

 public:
  explicit MinSparse(Engine&, VarId output, std::vector<VarId> varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
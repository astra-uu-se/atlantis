#pragma once

#include <unordered_map>

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {
/**
 * This is a violation invariant.
 * The violation variable is the minimum of all output variables.
 * Therefore, this invariant should be combined with a min invariant
 */
class BoolTableIn : public Invariant {
 protected:
  std::vector<VarViewId> _varArray;
  std::vector<std::vector<bool>> _transposed;
  std::vector<VarId> _rowViolations;

 public:
  explicit BoolTableIn(SolverBase&, std::vector<VarId>&& rowViolations,
                        std::vector<VarViewId>&& vars, const std::vector<std::vector<bool>>& table);

  explicit BoolTableIn(SolverBase&, std::vector<VarViewId>&& rowViolations,
                        std::vector<VarViewId>&& vars, const std::vector<std::vector<bool>>& table);


  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

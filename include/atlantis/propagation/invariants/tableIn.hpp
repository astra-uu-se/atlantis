#pragma once

#include <unordered_map>

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {
/**
 * This is a violation invariant.
 * The violation variable is the minimum of all output variables.
 * Therefore, this invariant should be combined with a min invariant
 */
class TableIn : public Invariant {
 protected:
  std::vector<VarViewId> _varArray;
  std::vector<std::unordered_map<Int, std::vector<size_t>>> _valToRows;
  std::vector<VarId> _rowViolations;

 public:
  explicit TableIn(SolverBase&, std::vector<VarId>&& rowViolations,
                        std::vector<VarViewId>&& vars, const std::vector<std::vector<Int>>& table);

  explicit TableIn(SolverBase&, std::vector<VarViewId>&& rowViolations,
                        std::vector<VarViewId>&& vars, const std::vector<std::vector<Int>>& table);


  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

#pragma once

#include <array>

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {
/**
 * This is a violation invariant.
 * The violation variable is the minimum of all output variables.
 * Therefore, this invariant should be combined with a min invariant
 */
class BoolTable : public Invariant {
 protected:
  VarViewId _inputVar;
  std::vector<VarId> _outputVars;
  std::array<std::vector<Int>, 2> _table;

  void recompute(Timestamp, bool);

 public:
  explicit BoolTable(SolverBase&, std::vector<VarId>&& outputVars,
                     VarViewId inputVar,
                     std::array<std::vector<Int>, 2>&& table,
                     size_t inputColumn = 0);

  explicit BoolTable(SolverBase&, std::vector<VarViewId>&& outputVars,
                     VarViewId inputVar,
                     std::array<std::vector<Int>, 2>&& table,
                     size_t inputColumn = 0);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

#pragma once

#include <unordered_map>

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {
/**
 * This is a violation invariant.
 * The violation variable is the minimum of all output variables.
 * Therefore, this invariant should be combined with a min invariant
 */
class Table : public Invariant {
 protected:
  VarViewId _inputVar;
  std::vector<VarId> _outputVars;
  std::unordered_map<Int, size_t> _valToRow;
  std::vector<std::vector<Int>> _table;

  void recompute(Timestamp, bool);

 public:
  explicit Table(SolverBase&, std::vector<VarId>&& outputVars,
                 VarViewId inputVar, std::vector<std::vector<Int>>&& table,
                 size_t inputColumn = 0);

  explicit Table(SolverBase&, std::vector<VarViewId>&& outputVars,
                 VarViewId inputVar, std::vector<std::vector<Int>>&& table,
                 size_t inputColumn = 0);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

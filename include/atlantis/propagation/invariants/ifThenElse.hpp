#pragma once

#include <array>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- if _condition = 0 then _thenVar else _elseVar
 *
 */

class IfThenElse : public Invariant {
 private:
  VarId _output;
  VarViewId _condition;
  std::array<const VarViewId, 2> _branches;
  Int _committedViolation;

 public:
  explicit IfThenElse(SolverBase&, VarId output, VarViewId condition,
                      VarViewId thenVar, VarViewId elseVar);

  explicit IfThenElse(SolverBase&, VarViewId output, VarViewId condition,
                      VarViewId thenVar, VarViewId elseVar);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  [[nodiscard]] VarViewId dynamicInputVar(Timestamp) const noexcept override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void commit(Timestamp) override;
};

}  // namespace atlantis::propagation

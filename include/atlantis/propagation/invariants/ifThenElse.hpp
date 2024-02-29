#pragma once

#include <array>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- if b = 0 then x else y
 *
 */

class IfThenElse : public Invariant {
 private:
  VarId _output, _b;
  std::array<const VarId, 2> _xy;

 public:
  explicit IfThenElse(SolverBase&, VarId output, VarId b, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  [[nodiscard]] VarId dynamicInputVar(Timestamp) const noexcept override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

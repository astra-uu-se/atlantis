#pragma once

#include <array>

#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- if b = 0 then x else y
 *
 */

class IfThenElse : public Invariant {
 private:
  const VarId _output, _b;
  const std::array<const VarId, 2> _xy;

 public:
  explicit IfThenElse(Engine&, VarId output, VarId b, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  VarId dynamicInputVar(Timestamp) const noexcept override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
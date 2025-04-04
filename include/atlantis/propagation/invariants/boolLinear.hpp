#pragma once

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- sum(coeffs_i * violArray_i)
 *
 */

class BoolLinear : public Invariant {
  VarId _output;
  std::vector<Int> _coeffs;
  std::vector<VarViewId> _violArray;

 public:
  explicit BoolLinear(SolverBase&, VarViewId output,
                      std::vector<VarViewId>&& violArray);

  explicit BoolLinear(SolverBase&, VarId output,
                      std::vector<VarViewId>&& violArray);

  explicit BoolLinear(SolverBase&, VarViewId output, std::vector<Int>&& coeffs,
                      std::vector<VarViewId>&& violArray);

  explicit BoolLinear(SolverBase&, VarId output, std::vector<Int>&& coeffs,
                      std::vector<VarViewId>&& violArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

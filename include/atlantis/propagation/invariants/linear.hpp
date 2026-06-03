#pragma once

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- sum(coeffs_i * varArray_i)
 *
 */

class Linear : public Invariant {
  Int _outputOffset;
  VarId _output;
  std::vector<Int> _coeffs;
  std::vector<VarViewId> _varArray;

 public:
  explicit Linear(SolverBase&, VarViewId output,
                  std::vector<VarViewId>&& varArray, Int outputOffset = 0);

  explicit Linear(SolverBase&, VarId output, std::vector<VarViewId>&& varArray, Int outputOffset = 0);

  explicit Linear(SolverBase&, VarViewId output, std::vector<Int>&& coeffs,
                  std::vector<VarViewId>&& varArray, Int outputOffset = 0);

  explicit Linear(SolverBase&, VarId output, std::vector<Int>&& coeffs,
                  std::vector<VarViewId>&& varArray, Int outputOffset = 0);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

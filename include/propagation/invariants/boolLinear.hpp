#pragma once

#include <utility>
#include <vector>

#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

class SolverBase;

/**
 * Invariant for output <- sum(coeffs_i * violArray_i)
 *
 */

class BoolLinear : public Invariant {
 private:
  const VarId _output;
  const std::vector<Int> _coeffs;
  const std::vector<VarId> _violArray;
  // (_isSatisfied[i] == 1) <=> (_violArray[i] == 0)
  std::vector<Int> _isSatisfied;

 public:
  explicit BoolLinear(SolverBase&, VarId output,
                      const std::vector<VarId>& violArray);
  explicit BoolLinear(SolverBase&, VarId output, std::vector<Int> coeffs,
                      std::vector<VarId> violArray);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
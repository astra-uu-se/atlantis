#pragma once

#include <utility>
#include <vector>

#include "propagation/invariants/invariant.hpp"
#include "propagation/solver.hpp"
#include "types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- sum(coeffs_i * violArray_i)
 *
 */

class BoolLinear : public Invariant {
 private:
  VarId _output;
  std::vector<Int> _coeffs;
  std::vector<VarId> _violArray;
  // (_isSatisfied[i] == 1) <=> (_violArray[i] == 0)
  std::vector<Int> _isSatisfied;

 public:
  explicit BoolLinear(SolverBase&, VarId output,
                      std::vector<VarId>&& violArray);
  explicit BoolLinear(SolverBase&, VarId output, std::vector<Int>&& coeffs,
                      std::vector<VarId>&& violArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
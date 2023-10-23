#pragma once

#include <utility>
#include <vector>

#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

class SolverBase;

/**
 * Invariant for output <- sum(coeffs_i * varArray_i)
 *
 */

class Linear : public Invariant {
 private:
  const VarId _output;
  const std::vector<Int> _coeffs;
  const std::vector<VarId> _varArray;
  std::vector<Int> _committedValues;

 public:
  explicit Linear(SolverBase&, VarId output, const std::vector<VarId>& varArray);
  explicit Linear(SolverBase&, VarId output, std::vector<Int> coeffs,
                  std::vector<VarId> varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
#pragma once

#include <utility>
#include <vector>

#include "propagation/invariants/invariant.hpp"
#include "propagation/solver.hpp"
#include "types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- sum(coeffs_i * varArray_i)
 *
 */

class Linear : public Invariant {
 private:
  VarId _output;
  std::vector<Int> _coeffs;
  std::vector<VarId> _varArray;
  std::vector<Int> _committedValues;

 public:
  explicit Linear(SolverBase&, VarId output, std::vector<VarId>&& varArray);
  explicit Linear(SolverBase&, VarId output, std::vector<Int>&& coeffs,
                  std::vector<VarId>&& varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
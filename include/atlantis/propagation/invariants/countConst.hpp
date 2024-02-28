#pragma once

#include <utility>
#include <vector>

#include "propagation/invariants/invariant.hpp"
#include "propagation/solver.hpp"
#include "types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for:
 * output <- number of occurences of _denominator in _vars
 *
 */

class CountConst : public Invariant {
 private:
  VarId _output;
  Int _y;
  std::vector<VarId> _vars;
  std::vector<Int> _hasCountValue;

 public:
  explicit CountConst(SolverBase&, VarId output, Int y,
                      std::vector<VarId>&& vars);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
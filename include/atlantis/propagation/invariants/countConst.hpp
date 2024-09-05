#pragma once

#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for:
 * output <- number of occurences of needle in _vars
 *
 */

class CountConst : public Invariant {
 private:
  VarId _output;
  Int _needle;
  std::vector<VarViewId> _vars;

 public:
  explicit CountConst(SolverBase&, VarId output, Int needle,
                      std::vector<VarViewId>&& vars);

  explicit CountConst(SolverBase&, VarViewId output, Int needle,
                      std::vector<VarViewId>&& vars);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

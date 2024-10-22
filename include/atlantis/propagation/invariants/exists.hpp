#pragma once

#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- exists b == 0 in varArray
 *
 */

class Exists : public Invariant {
 private:
  VarId _output;
  std::vector<VarViewId> _varArray;
  CommittableInt _minIndex;

 public:
  explicit Exists(SolverBase&, VarId output, std::vector<VarViewId>&& varArray);

  explicit Exists(SolverBase&, VarViewId output,
                  std::vector<VarViewId>&& varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

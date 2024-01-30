#pragma once

#include <limits>
#include <utility>
#include <vector>

#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/invariants/invariant.hpp"
#include "propagation/utils/priorityList.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- exists b == 0 in varArray
 *
 */

class Exists : public Invariant {
 private:
  VarId _output;
  std::vector<VarId> _varArray;
  CommittableInt _min;

 public:
  explicit Exists(SolverBase&, VarId output, std::vector<VarId>&& varArray);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
#pragma once

#include <cassert>
#include <limits>
#include <vector>

#include "constraint.hpp"
#include "propagation/solver.hpp"
#include "propagation/variables/committableInt.hpp"
#include "propagation/variables/intVar.hpp"
#include "types.hpp"

namespace atlantis::propagation {

class BoolAllEqual : public Constraint {
 protected:
  std::vector<VarId> _vars;
  std::vector<Int> _committedValues;
  CommittableInt _numTrue;

 public:
  explicit BoolAllEqual(SolverBase&, VarId violationId,
                        std::vector<VarId> vars);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
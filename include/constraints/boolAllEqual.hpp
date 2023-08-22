#pragma once

#include <cassert>
#include <limits>
#include <vector>

#include "constraint.hpp"
#include "core/engine.hpp"
#include "core/types.hpp"
#include "variables/committableInt.hpp"
#include "variables/intVar.hpp"

class BoolAllEqual : public Constraint {
 protected:
  std::vector<VarId> _variables;
  std::vector<Int> _committedValues;
  CommittableInt _numTrue;

 public:
  explicit BoolAllEqual(Engine&, VarId violationId,
                        std::vector<VarId> variables);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

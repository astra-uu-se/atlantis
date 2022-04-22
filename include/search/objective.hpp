#pragma once

#include <limits>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariants/linear.hpp"
#include "search/assignment.hpp"

namespace search {

class Objective {
 private:
  PropagationEngine& _engine;
  const ObjectiveDirection _objectiveDirection;
  const VarId _violation;
  const VarId _objective;
  const VarId _bound;

 public:
  Objective(PropagationEngine& engine, ObjectiveDirection objectiveDirection,
            VarId objective, VarId violation);
  Objective(PropagationEngine& engine, ObjectiveDirection objectiveDirection,
            VarId objective, VarId violation, VarId bound);

  void tighten();

  [[nodiscard]] VarId bound() const noexcept { return _bound; }
  [[nodiscard]] VarId violation() const noexcept { return _violation; }
  [[nodiscard]] VarId objective() const noexcept { return _objective; }
  [[nodiscard]] ObjectiveDirection objectiveDirection() const noexcept {
    return _objectiveDirection;
  }

  [[nodiscard]] static Objective createAndRegister(
      PropagationEngine& engine, ObjectiveDirection objectiveDirection,
      invariantgraph::InvariantGraphApplyResult& applicationResult);

  [[nodiscard]] static Objective createAndRegister(
      PropagationEngine& engine, ObjectiveDirection objectiveDirection,
      VarId totalViolations, VarId objectiveVariable);

  [[nodiscard]] search::Assignment createAssignment();
};

}  // namespace search

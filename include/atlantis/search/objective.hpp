#pragma once

#include <functional>

#include "atlantis/propagation/solver.hpp"
#include "cost.hpp"

namespace atlantis::search {

class Objective {
  propagation::Solver& _solver;
  ObjectiveDirection _problemType;

  propagation::VarViewId _bound{propagation::NULL_ID};
  propagation::VarViewId _objective{propagation::NULL_ID};
  propagation::VarId _violation{propagation::NULL_ID};

 public:
  Objective(propagation::Solver& solver, ObjectiveDirection objectiveDirection);

  propagation::VarViewId registerNode(
      propagation::VarViewId totalViolationVarId,
      propagation::VarViewId objectiveVarId);

  void tighten();

  // A clone of the above function that uses a supplied cost.
  // This assumes the violation is 0.
  void tighten(const Cost& cost);

  [[nodiscard]] propagation::VarViewId bound() const noexcept;
};

}  // namespace atlantis::search

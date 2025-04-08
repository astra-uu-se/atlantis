#pragma once

#include <functional>
#include <fznparser/solveType.hpp>

#include "atlantis/propagation/solver.hpp"

namespace atlantis::search {

class Objective {
  propagation::Solver& _solver;
  fznparser::ProblemType _problemType;

  propagation::VarViewId _bound{propagation::NULL_ID};
  propagation::VarViewId _objective{propagation::NULL_ID};
  propagation::VarId _violation{propagation::NULL_ID};

 public:
  Objective(propagation::Solver& solver, fznparser::ProblemType problemType);

  propagation::VarViewId registerNode(
      propagation::VarViewId totalViolationVarId,
      propagation::VarViewId objectiveVarId);

  void tighten();

  [[nodiscard]] propagation::VarViewId bound() const noexcept;

};

}  // namespace atlantis::search

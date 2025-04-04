#pragma once

#include <functional>
#include <fznparser/solveType.hpp>

#include "atlantis/propagation/solver.hpp"

namespace atlantis::search {

class Objective {
  propagation::Solver& _solver;
  fznparser::ProblemType _problemType;

  std::optional<propagation::VarViewId> _bound{};
  std::optional<propagation::VarViewId> _objective{};
  std::optional<propagation::VarId> _violation{};

 public:
  Objective(propagation::Solver& solver, fznparser::ProblemType problemType);

  propagation::VarViewId registerNode(
      propagation::VarViewId totalViolationVarId,
      propagation::VarViewId objectiveVarId);

  void tighten();

  [[nodiscard]] std::optional<propagation::VarViewId> bound() const noexcept;

 private:
  propagation::VarViewId registerOptimisation(
      propagation::VarViewId constraintViolation,
      propagation::VarViewId objectiveVarId, Int initialBound,
      std::function<void(propagation::VarId, propagation::VarViewId)>&&
          constraintFactory);
};

}  // namespace atlantis::search

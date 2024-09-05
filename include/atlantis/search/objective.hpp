#pragma once

#include <fznparser/solveType.hpp>
#include <limits>

#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"
#include "atlantis/utils/variant.hpp"

namespace atlantis::search {

class Objective {
 private:
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

  [[nodiscard]] std::optional<propagation::VarViewId> bound() const noexcept {
    return _bound;
  }

 private:
  template <typename F>
  propagation::VarViewId registerOptimisation(
      propagation::VarViewId constraintViolation,
      propagation::VarViewId objectiveVarId, Int initialBound,
      F constraintFactory) {
    _bound =
        _solver.makeIntVar(initialBound, _solver.lowerBound(objectiveVarId),
                           _solver.upperBound(objectiveVarId));

    const auto boundViolation = propagation::VarId(
        _solver.makeIntVar(0, 0, std::numeric_limits<Int>::max()));

    constraintFactory(boundViolation, *_bound);

    if (constraintViolation == propagation::NULL_ID) {
      _violation = boundViolation;
    } else {
      _violation = propagation::VarId(
          _solver.makeIntVar(0, 0, std::numeric_limits<Int>::max()));

      _solver.makeInvariant<propagation::Linear>(
          _solver, *_violation,
          std::vector<propagation::VarViewId>{boundViolation,
                                              constraintViolation});
    }
    return *_violation;
  }
};

}  // namespace atlantis::search

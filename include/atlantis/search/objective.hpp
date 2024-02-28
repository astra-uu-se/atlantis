#pragma once

#include <limits>
#include <utility>

#include "fznparser/model.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/solver.hpp"
#include "propagation/types.hpp"
#include "propagation/violationInvariants/lessEqual.hpp"
#include "types.hpp"
#include "utils/variant.hpp"

namespace atlantis::search {

class Objective {
 private:
  propagation::Solver& _solver;
  fznparser::ProblemType _problemType;

  std::optional<propagation::VarId> _bound{};
  std::optional<propagation::VarId> _objective{};
  std::optional<propagation::VarId> _violation{};

 public:
  Objective(propagation::Solver& solver, fznparser::ProblemType problemType);

  propagation::VarId registerNode(propagation::VarId totalViolationVarId,
                                  propagation::VarId objectiveVarId);

  void tighten();

  [[nodiscard]] std::optional<propagation::VarId> bound() const noexcept {
    return _bound;
  }

 private:
  template <typename F>
  propagation::VarId registerOptimisation(
      propagation::VarId constraintViolation, propagation::VarId objectiveVarId,
      Int initialBound, F constraintFactory) {
    auto lb = _solver.lowerBound(objectiveVarId);
    auto ub = _solver.upperBound(objectiveVarId);

    _bound = _solver.makeIntVar(initialBound, lb, ub);
    auto boundViolation =
        _solver.makeIntVar(0, 0, std::numeric_limits<Int>::max());
    constraintFactory(boundViolation, *_bound);

    if (constraintViolation == propagation::NULL_ID) {
      _violation = boundViolation;
    } else {
      _violation = _solver.makeIntVar(0, 0, std::numeric_limits<Int>::max());
      _solver.makeInvariant<propagation::Linear>(
          _solver, *_violation,
          std::vector<propagation::VarId>{boundViolation, constraintViolation});
    }
    return *_violation;
  }
};

}  // namespace atlantis::search

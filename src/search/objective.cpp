#include "atlantis/search/objective.hpp"

#include <limits>
#include <utility>

#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include <fznparser/model.hpp>

namespace atlantis::search {

Objective::Objective(propagation::Solver& solver,
                     fznparser::ProblemType problemType)
    : _solver(solver), _problemType(problemType) {}

propagation::VarId Objective::registerNode(
    propagation::VarId totalViolationVarId, propagation::VarId objectiveVarId) {
  assert(_solver.isOpen());

  _objective = objectiveVarId;

  if (_problemType == fznparser::ProblemType::MINIMIZE) {
    return registerOptimisation(
        totalViolationVarId, objectiveVarId, _solver.upperBound(objectiveVarId),
        [&](propagation::VarId v, propagation::VarId b) {
          _solver.makeViolationInvariant<propagation::LessEqual>(
              _solver, v, objectiveVarId, b);
        });
  } else if (_problemType == fznparser::ProblemType::MAXIMIZE) {
    return registerOptimisation(
        totalViolationVarId, objectiveVarId, _solver.lowerBound(objectiveVarId),
        [&](propagation::VarId v, propagation::VarId b) {
          _solver.makeViolationInvariant<propagation::LessEqual>(
              _solver, v, b, objectiveVarId);
        });
  }
  assert(_problemType == fznparser::ProblemType::SATISFY);
  return totalViolationVarId;
}

void Objective::tighten() {
  if (!_bound) {
    return;
  }

  const Int newBound =
      _problemType == fznparser::ProblemType::SATISFY
          ? _solver.committedValue(*_bound)
          : (_solver.committedValue(*_objective) +
             (_problemType == fznparser::ProblemType::MINIMIZE ? -1 : 1));

  _solver.beginMove();
  _solver.setValue(*_bound, newBound);
  _solver.endMove();

  _solver.beginCommit();
  _solver.query(*_violation);
  _solver.endCommit();
}

}  // namespace atlantis::search

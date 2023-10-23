#include "search/objective.hpp"

#include "propagation/constraints/lessEqual.hpp"
#include "utils/variant.hpp"

namespace atlantis::search {

Objective::Objective(propagation::PropagationEngine& engine,
                     fznparser::ProblemType problemType)
    : _engine(engine), _problemType(problemType) {}

propagation::VarId Objective::registerNode(propagation::VarId totalViolationId,
                                           propagation::VarId objectiveVarId) {
  assert(_engine.isOpen());

  _objective = objectiveVarId;

  if (_problemType == fznparser::ProblemType::MINIMIZE) {
    return registerOptimisation(
        totalViolationId, objectiveVarId, _engine.upperBound(objectiveVarId),
        [&](propagation::VarId v, propagation::VarId b) {
          _engine.makeConstraint<propagation::LessEqual>(_engine, v,
                                                         objectiveVarId, b);
        });
  } else if (_problemType == fznparser::ProblemType::MAXIMIZE) {
    return registerOptimisation(
        totalViolationId, objectiveVarId, _engine.lowerBound(objectiveVarId),
        [&](propagation::VarId v, propagation::VarId b) {
          _engine.makeConstraint<propagation::LessEqual>(_engine, v, b,
                                                         objectiveVarId);
        });
  }
  assert(_problemType == fznparser::ProblemType::SATISFY);
  return totalViolationId;
}

void Objective::tighten() {
  if (!_bound) {
    return;
  }

  const Int newBound =
      _problemType == fznparser::ProblemType::SATISFY
          ? _engine.committedValue(*_bound)
          : (_engine.committedValue(*_objective) +
             (_problemType == fznparser::ProblemType::MINIMIZE ? -1 : 1));

  _engine.beginMove();
  _engine.setValue(*_bound, newBound);
  _engine.endMove();

  _engine.beginCommit();
  _engine.query(*_violation);
  _engine.endCommit();
}

}  // namespace atlantis::search
#include "search/objective.hpp"

#include "constraints/lessEqual.hpp"
#include "utils/variant.hpp"

search::Objective::Objective(PropagationEngine& engine,
                             fznparser::ProblemType problemType)
    : _engine(engine), _problemType(problemType) {}

VarId search::Objective::registerNode(VarId totalViolationId,
                                      VarId objectiveVarId) {
  assert(_engine.isOpen());

  _objective = objectiveVarId;

  if (_problemType == fznparser::ProblemType::MINIMIZE) {
    return registerOptimisation(
        totalViolationId, objectiveVarId, _engine.upperBound(objectiveVarId),
        [&](VarId v, VarId b) {
          _engine.makeConstraint<LessEqual>(_engine, v, objectiveVarId, b);
        });
  } else if (_problemType == fznparser::ProblemType::MAXIMIZE) {
    return registerOptimisation(
        totalViolationId, objectiveVarId, _engine.lowerBound(objectiveVarId),
        [&](VarId v, VarId b) {
          _engine.makeConstraint<LessEqual>(_engine, v, b, objectiveVarId);
        });
  }
  assert(_problemType == fznparser::ProblemType::SATISFY);
  return totalViolationId;
}

void search::Objective::tighten() {
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

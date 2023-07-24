#include "search/objective.hpp"

#include "constraints/lessEqual.hpp"
#include "utils/variant.hpp"

search::Objective::Objective(PropagationEngine& engine,
                             const fznparser::Model& model)
    : _engine(engine), _modelObjective(model.objective()) {}

VarId search::Objective::registerWithEngine(VarId constraintViolation,
                                            VarId objectiveVariable) {
  assert(_engine.isOpen());

  _objective = objectiveVariable;

  return std::visit<VarId>(
      overloaded{
          [&](const fznparser::Satisfy&) { return constraintViolation; },
          [&](const fznparser::Minimise&) {
            VarId violation = registerOptimisation(
                constraintViolation, objectiveVariable,
                _engine.upperBound(objectiveVariable), [&](VarId v, VarId b) {
                  _engine.makeConstraint<LessEqual>(_engine, v,
                                                    objectiveVariable, b);
                });
            return violation;
          },
          [&](const fznparser::Maximise&) {
            VarId violation = registerOptimisation(
                constraintViolation, objectiveVariable,
                _engine.lowerBound(objectiveVariable), [&](VarId v, VarId b) {
                  _engine.makeConstraint<LessEqual>(_engine, v, b,
                                                    objectiveVariable);
                });
            return violation;
          },
      },
      _modelObjective);
}

void search::Objective::tighten() {
  if (!_bound) {
    return;
  }

  auto newBound = std::visit<Int>(
      overloaded{[&](const fznparser::Satisfy&) {
                   return _engine.committedValue(*_bound);
                 },
                 [&](const fznparser::Minimise&) {
                   return _engine.committedValue(*_objective) - 1;
                 },
                 [&](const fznparser::Maximise&) {
                   return _engine.committedValue(*_objective) + 1;
                 }},
      _modelObjective);

  _engine.beginMove();
  _engine.setValue(*_bound, newBound);
  _engine.endMove();

  _engine.beginCommit();
  _engine.query(*_violation);
  _engine.endCommit();
}

#include "invariantgraph/constraints/boolClauseNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/linear.hpp"
#include "views/bool2IntView.hpp"
#include "views/equalView.hpp"
#include "views/notEqualView.hpp"

std::unique_ptr<invariantgraph::BoolClauseNode>
invariantgraph::BoolClauseNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "bool_clause" && constraint.arguments.size() == 2) ||
      (constraint.name == "bool_clause_reif" &&
       constraint.arguments.size() == 3));

  VariableNode* r = constraint.arguments.size() >= 3
                        ? mappedVariable(constraint.arguments[2], variableMap)
                        : nullptr;

  return std::make_unique<BoolClauseNode>(
      mappedVariableVector(model, constraint.arguments[0], variableMap),
      mappedVariableVector(model, constraint.arguments[1], variableMap), r);
}

void invariantgraph::BoolClauseNode::createDefinedVariables(Engine& engine) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    assert(violationVarId() == NULL_ID);
    setViolationVarId(engine.makeIntView<EqualView>(
        _sumVarId,
        static_cast<Int>(_as.size()) + static_cast<Int>(_bs.size())));
  }
}

void invariantgraph::BoolClauseNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> engineVariables;
  engineVariables.reserve(_as.size() + _bs.size());
  std::transform(_as.begin(), _as.end(), std::back_inserter(engineVariables),
                 [&](const auto& var) { return var->varId(); });

  std::transform(_bs.begin(), _bs.end(), std::back_inserter(engineVariables),
                 [&](const auto& var) {
                   return engine.makeIntView<NotEqualView>(var->varId(), 0);
                 });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);
  engine.makeInvariant<Linear>(engineVariables, _sumVarId);
}

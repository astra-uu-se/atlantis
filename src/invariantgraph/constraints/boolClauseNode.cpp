#include "invariantgraph/constraints/boolClauseNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/boolLinear.hpp"
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

  std::vector<invariantgraph::VariableNode*> as =
      mappedVariableVector(model, constraint.arguments[0], variableMap);
  std::vector<invariantgraph::VariableNode*> bs =
      mappedVariableVector(model, constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::BoolClauseNode>(as, bs,
                                                              shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::BoolClauseNode>(as, bs, r);
    }
  }
  return std::make_unique<BoolClauseNode>(as, bs, true);
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
  engine.makeInvariant<BoolLinear>(engineVariables, _sumVarId);
}

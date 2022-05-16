#include "invariantgraph/constraints/linEqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"
#include "invariants/linear.hpp"
#include "views/bool2IntView.hpp"
#include "views/equalView.hpp"

std::unique_ptr<invariantgraph::LinEqNode>
invariantgraph::LinEqNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      ((constraint.name == "int_lin_eq" || constraint.name == "bool_lin_eq") &&
       constraint.arguments.size() == 3) ||
      ((constraint.name == "int_lin_eq_reif" ||
        constraint.name == "bool_lin_eq_reif") &&
       constraint.arguments.size() == 4));

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto bound = integerValue(model, constraint.arguments[2]);

  VariableNode* r = constraint.arguments.size() >= 4
                        ? mappedVariable(constraint.arguments[3], variableMap)
                        : nullptr;

  return std::make_unique<LinEqNode>(coeffs, variables, bound, r);
}

void invariantgraph::LinEqNode::createDefinedVariables(Engine& engine) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    assert(violationVarId() == NULL_ID);
    setViolationVarId(engine.makeIntView<EqualView>(_sumVarId, _c));
  }
}

void invariantgraph::LinEqNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](auto node) { return node->varId(); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<Linear>(_coeffs, variables, _sumVarId);
}

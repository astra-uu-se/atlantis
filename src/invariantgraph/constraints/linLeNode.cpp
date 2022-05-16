#include "invariantgraph/constraints/linLeNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "constraints/lessEqual.hpp"
#include "invariants/linear.hpp"
#include "views/bool2IntView.hpp"
#include "views/lessEqualView.hpp"

std::unique_ptr<invariantgraph::LinLeNode>
invariantgraph::LinLeNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      ((constraint.name == "int_lin_le" || constraint.name == "bool_lin_le") &&
       constraint.arguments.size() == 3) ||
      ((constraint.name == "int_lin_le_reif" ||
        constraint.name == "bool_lin_le_reif") &&
       constraint.arguments.size() == 4));

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto bound = integerValue(model, constraint.arguments[2]);

  VariableNode* r = constraint.arguments.size() >= 4
                        ? mappedVariable(constraint.arguments[3], variableMap)
                        : nullptr;

  return std::make_unique<LinLeNode>(coeffs, variables, bound, r);
}

void invariantgraph::LinLeNode::createDefinedVariables(Engine& engine) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    assert(violationVarId() == NULL_ID);
    setViolationVarId(engine.makeIntView<LessEqualView>(_sumVarId, _bound));
  }
}

void invariantgraph::LinLeNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](auto node) { return node->varId(); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<Linear>(_coeffs, variables, _sumVarId);
}

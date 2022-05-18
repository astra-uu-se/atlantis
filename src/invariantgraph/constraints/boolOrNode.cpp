#include "invariantgraph/constraints/boolOrNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolOrNode>
invariantgraph::BoolOrNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "bool_or" && constraint.arguments.size() == 2) ||
      (constraint.name == "bool_or_reif" && constraint.arguments.size() == 3));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::BoolOrNode>(a, b, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::BoolOrNode>(a, b, r);
    }
  }
  return std::make_unique<BoolOrNode>(a, b, true);
}

void invariantgraph::BoolOrNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    if (shouldHold()) {
      registerViolation(engine);
    } else {
      assert(!isReified());
      _intermediate = engine.makeIntVar(0, 0, 0);
      setViolationVarId(engine.makeIntView<NotEqualView>(_intermediate, 0));
    }
  }
}

void invariantgraph::BoolOrNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<BoolOr>(a()->varId(), b()->varId(), violationVarId());
}
#include "invariantgraph/constraints/intEqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"
#include "constraints/notEqual.hpp"

std::unique_ptr<invariantgraph::IntEqNode>
invariantgraph::IntEqNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "int_eq" && constraint.arguments.size() == 2) ||
      (constraint.name == "int_eq_reif" && constraint.arguments.size() == 3));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::IntEqNode>(a, b, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::IntEqNode>(a, b, r);
    }
  }
  return std::make_unique<IntEqNode>(a, b, true);
}

void invariantgraph::IntEqNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::IntEqNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<Equal>(violationVarId(), a()->varId(), b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<NotEqual>(violationVarId(), a()->varId(),
                                    b()->varId());
  }
}
#include "invariantgraph/constraints/boolAndNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolAndNode>
invariantgraph::BoolAndNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::BoolAndNode>(a, b, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::BoolAndNode>(a, b, r);
    }
  }
  return std::make_unique<BoolAndNode>(a, b, true);
}

void invariantgraph::BoolAndNode::createDefinedVariables(Engine& engine) {
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

void invariantgraph::BoolAndNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  engine.makeInvariant<BoolAnd>(violationVarId(), a()->varId(), b()->varId());
}
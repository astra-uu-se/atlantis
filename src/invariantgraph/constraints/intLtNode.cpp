#include "invariantgraph/constraints/intLtNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntLtNode>
invariantgraph::IntLtNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::IntLtNode>(a, b, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::IntLtNode>(a, b, r);
    }
  }
  return std::make_unique<IntLtNode>(a, b, true);
}

void invariantgraph::IntLtNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::IntLtNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<LessThan>(engine, violationVarId(), a()->varId(),
                                    b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<LessEqual>(engine, violationVarId(), b()->varId(),
                                     a()->varId());
  }
}
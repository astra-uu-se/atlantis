#include "invariantgraph/invariants/intDivNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntDivNode>
invariantgraph::IntDivNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<IntDivNode>(a, b, output);
}

void invariantgraph::IntDivNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::IntDivNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<IntDiv>(a()->varId(), b()->varId(),
                               definedVariables().front()->varId());
}
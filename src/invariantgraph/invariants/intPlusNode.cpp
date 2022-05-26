#include "invariantgraph/invariants/intPlusNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntPlusNode>
invariantgraph::IntPlusNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<IntPlusNode>(a, b, output);
}

void invariantgraph::IntPlusNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::IntPlusNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<Plus>(a()->varId(), b()->varId(),
                             definedVariables().front()->varId());
}
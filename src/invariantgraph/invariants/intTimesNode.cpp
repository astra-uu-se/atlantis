#include "invariantgraph/invariants/intTimesNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntTimesNode>
invariantgraph::IntTimesNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<IntTimesNode>(a, b, output);
}

void invariantgraph::IntTimesNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::IntTimesNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<Times>(definedVariables().front()->varId(), a()->varId(),
                              b()->varId());
}
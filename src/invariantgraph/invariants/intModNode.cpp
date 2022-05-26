#include "invariantgraph/invariants/intModNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntModNode>
invariantgraph::IntModNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<IntModNode>(a, b, output);
}

void invariantgraph::IntModNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::IntModNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<Mod>(a()->varId(), b()->varId(),
                            definedVariables().front()->varId());
}
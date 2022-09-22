#include "invariantgraph/invariants/intPowNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntPowNode>
invariantgraph::IntPowNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<IntPowNode>(a, b, output);
}

void invariantgraph::IntPowNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::IntPowNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId(this) != NULL_ID);
  engine.makeInvariant<Pow>(definedVariables().front()->varId(this),
                            a()->inputVarId(), b()->inputVarId());
}
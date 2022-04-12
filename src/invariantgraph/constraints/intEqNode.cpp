#include "invariantgraph/constraints/intEqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"

std::unique_ptr<invariantgraph::IntEqNode>
invariantgraph::IntEqNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<IntEqNode>(a, b);
}

void invariantgraph::IntEqNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  VarId violation = registerViolation(engine, variableMap);

  engine.makeConstraint<::Equal>(violation, variableMap.at(_a),
                                 variableMap.at(_b));
}
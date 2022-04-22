#include "invariantgraph/constraints/eqNode.hpp"

#include "constraints/equal.hpp"
#include "invariantgraph/parseHelper.hpp"

std::unique_ptr<invariantgraph::EqNode>
invariantgraph::EqNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<EqNode>(a, b);
}

void invariantgraph::EqNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  VarId violation = registerViolation(engine, variableMap);

  engine.makeConstraint<::Equal>(violation, variableMap.at(_a),
                                 variableMap.at(_b));
}
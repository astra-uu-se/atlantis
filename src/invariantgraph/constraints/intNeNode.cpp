#include "invariantgraph/constraints/intNeNode.hpp"

#include "constraints/notEqual.hpp"
#include "invariantgraph/parseHelper.hpp"

std::unique_ptr<invariantgraph::IntNeNode>
invariantgraph::IntNeNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_ne");
  assert(constraint.arguments.size() == 2);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<IntNeNode>(a, b);
}

void invariantgraph::IntNeNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  VarId violation = registerViolation(engine, variableMap);

  engine.makeConstraint<::NotEqual>(violation, variableMap.at(_a),
                                    variableMap.at(_b));
}
#include "invariantgraph/invariants/binaryOpNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/invariants/intDivNode.hpp"
#include "invariantgraph/invariants/intModNode.hpp"
#include "invariantgraph/invariants/intPowNode.hpp"
#include "invariantgraph/invariants/intTimesNode.hpp"

template <typename T>
std::unique_ptr<T> invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  static_assert(std::is_base_of<BinaryOpNode, T>{});

  assert(constraint.name == T::constraint_name());
  assert(constraint.arguments.size() == 3);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<T>(a, b, output);
}

void invariantgraph::BinaryOpNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(definedVariables()[0])) {
    registerDefinedVariable(engine, variableMap, definedVariables()[0]);
  }
}

void invariantgraph::BinaryOpNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  assert(variableMap.contains(definedVariables()[0]));
  createInvariant(engine, variableMap.at(a()), variableMap.at(b()),
                  variableMap.at(definedVariables()[0]));
}

// Instantiation of the binary operator factory methods.
template std::unique_ptr<invariantgraph::IntDivNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntModNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntTimesNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntPowNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

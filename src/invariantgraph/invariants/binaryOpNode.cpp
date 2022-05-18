#include "invariantgraph/invariants/binaryOpNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/invariants/intDivNode.hpp"
#include "invariantgraph/invariants/intMaxNode.hpp"
#include "invariantgraph/invariants/intMinNode.hpp"
#include "invariantgraph/invariants/intModNode.hpp"
#include "invariantgraph/invariants/intPlusNode.hpp"
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

void invariantgraph::BinaryOpNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::BinaryOpNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  createInvariant(engine, a()->varId(), b()->varId(),
                  definedVariables().front()->varId());
}

// Instantiation of the binary operator factory methods.
template std::unique_ptr<invariantgraph::IntDivNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntMaxNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntMinNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntModNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntPlusNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntPowNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);

template std::unique_ptr<invariantgraph::IntTimesNode>
invariantgraph::BinaryOpNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap);
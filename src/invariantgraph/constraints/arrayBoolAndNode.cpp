#include "invariantgraph/constraints/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/elementConst.hpp"
#include "invariants/forAll.hpp"
#include "views/violation2BoolView.hpp"

std::unique_ptr<invariantgraph::ArrayBoolAndNode>
invariantgraph::ArrayBoolAndNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_and");
  assert(constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);
  if (std::holds_alternative<bool>(constraint.arguments[1])) {
    auto value = std::get<bool>(constraint.arguments[1]);
    return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, value);
  } else {
    auto r = mappedVariable(constraint.arguments[1], variableMap);
    return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, r);
  }
}

void invariantgraph::ArrayBoolAndNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::ArrayBoolAndNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return variableMap.at(node); });

  // TODO: The case where _rValue is false.
  assert(!_rIsConstant || _rValue);
  engine.makeInvariant<ForAll>(inputs, variableMap.at(violation()));
}

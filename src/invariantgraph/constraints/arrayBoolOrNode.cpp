#include "invariantgraph/constraints/arrayBoolOrNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/exists.hpp"
#include "views/violation2BoolView.hpp"

std::unique_ptr<invariantgraph::ArrayBoolOrNode>
invariantgraph::ArrayBoolOrNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_or");
  assert(constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);

  if (std::holds_alternative<bool>(constraint.arguments[1])) {
    auto value = std::get<bool>(constraint.arguments[1]);
    return std::make_unique<invariantgraph::ArrayBoolOrNode>(as, value);
  } else {
    auto r = mappedVariable(constraint.arguments[1], variableMap);
    return std::make_unique<invariantgraph::ArrayBoolOrNode>(as, r);
  }
}

void invariantgraph::ArrayBoolOrNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::ArrayBoolOrNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return variableMap.at(node); });
#ifndef NDEBUG
  for (const VarId input : inputs) {
    assert(0 <= engine.lowerBound(input));
  }
#endif

  // TODO: The case where _rValue is false when r is constant.
  // For that a new invariant is needed, since a simple view performing negation
  // cannot know how far off the inputs are from all being non-zero.
  assert(!_rIsConstant || _rValue);
  engine.makeInvariant<Exists>(inputs, variableMap.at(violation()));
}

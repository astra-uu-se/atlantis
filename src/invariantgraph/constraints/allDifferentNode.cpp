#include "invariantgraph/constraints/allDifferentNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/allDifferent.hpp"

std::unique_ptr<invariantgraph::AllDifferentNode>
invariantgraph::AllDifferentNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "alldifferent" && constraint.arguments.size() == 1) ||
      (constraint.name == "alldifferent_reif" &&
       constraint.arguments.size() == 2));

  auto variables =
      mappedVariableVector(model, constraint.arguments[0], variableMap);

  VariableNode* r = constraint.arguments.size() >= 2
                        ? mappedVariable(constraint.arguments[1], variableMap)
                        : nullptr;

  return std::make_unique<AllDifferentNode>(variables, r);
}

void invariantgraph::AllDifferentNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::AllDifferentNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  assert(variableMap.contains(violation()));

  std::vector<VarId> engineVariables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(engineVariables),
                 [&](const auto& var) { return variableMap.at(var); });

  engine.makeConstraint<AllDifferent>(variableMap.at(violation()),
                                      engineVariables);
}

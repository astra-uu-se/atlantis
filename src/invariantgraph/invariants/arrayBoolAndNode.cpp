#include "invariantgraph/invariants/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/elementConst.hpp"
#include "invariants/linear.hpp"
#include "views/violation2BoolView.hpp"

std::unique_ptr<invariantgraph::ArrayBoolAndNode>
invariantgraph::ArrayBoolAndNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_and");
  assert(constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);
  auto r = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, r);
}

void invariantgraph::ArrayBoolAndNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> inputs;
  std::transform(_as.begin(), _as.end(), std::back_inserter(inputs),
                 [&](const auto& node) { return variableMap.at(node); });

  auto sum = engine.makeIntVar(0, 0, 0);
  engine.makeInvariant<Linear>(inputs, sum);

  auto output = engine.makeIntView<Violation2BoolView>(sum);
  variableMap.emplace(definedVariables()[0], output);
}

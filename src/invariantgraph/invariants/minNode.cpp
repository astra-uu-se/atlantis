#include "invariantgraph/invariants/minNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "invariants/minSparse.hpp"

std::unique_ptr<invariantgraph::MinNode>
invariantgraph::MinNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_int_minimum");
  assert(constraint.arguments.size() == 2);

  auto inputs =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[0], variableMap);

  return std::make_unique<invariantgraph::MinNode>(inputs, output);
}

void invariantgraph::MinNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::MinNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return node->varId(); });

  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<::MinSparse>(variables,
                                    definedVariables().front()->varId());
}

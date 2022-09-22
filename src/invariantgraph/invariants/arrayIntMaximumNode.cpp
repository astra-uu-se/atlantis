#include "invariantgraph/invariants/arrayIntMaximumNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "invariants/maxSparse.hpp"

std::unique_ptr<invariantgraph::ArrayIntMaximumNode>
invariantgraph::ArrayIntMaximumNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto inputs =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[0], variableMap);

  return std::make_unique<invariantgraph::ArrayIntMaximumNode>(inputs, output);
}

void invariantgraph::ArrayIntMaximumNode::createDefinedVariables(
    Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::ArrayIntMaximumNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return node->inputVarId(); });

  assert(definedVariables().front()->varId(this) != NULL_ID);
  engine.makeInvariant<MaxSparse>(definedVariables().front()->varId(this),
                                  variables);
}

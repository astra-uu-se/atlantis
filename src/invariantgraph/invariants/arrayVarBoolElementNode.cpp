#include "invariantgraph/invariants/arrayVarBoolElementNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/elementVar.hpp"

std::unique_ptr<invariantgraph::ArrayVarBoolElementNode>
invariantgraph::ArrayVarBoolElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_var_bool_element");
  assert(constraint.arguments.size() == 3);

  auto b = mappedVariable(constraint.arguments[0], variableMap);
  auto as = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::ArrayVarBoolElementNode>(as, b, c);
}

void invariantgraph::ArrayVarBoolElementNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(definedVariables()[0])) {
    registerDefinedVariable(engine, variableMap, definedVariables()[0]);
  }
}

void invariantgraph::ArrayVarBoolElementNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> as;
  std::transform(_as.begin(), _as.end(), std::back_inserter(as),
                 [&](auto var) { return variableMap.at(var); });

  assert(variableMap.contains(definedVariables()[0]));
  engine.makeInvariant<ElementVar>(variableMap.at(_b), as,
                                   variableMap.at(definedVariables()[0]));
}

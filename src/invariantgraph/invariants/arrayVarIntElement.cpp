#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"
#include "invariantgraph/parseHelper.hpp"
#include "invariants/elementVar.hpp"

std::unique_ptr<invariantgraph::ArrayVarIntElementNode>
invariantgraph::ArrayVarIntElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_var_int_element");
  assert(constraint.arguments.size() == 3);

  auto as = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto b = mappedVariable(constraint.arguments[0], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::ArrayVarIntElementNode>(as, b, c);
}

void invariantgraph::ArrayVarIntElementNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> as;
  std::transform(_as.begin(), _as.end(), std::back_inserter(as),
                 [&](auto var) { return variableMap.at(var); });

  auto outputId =
      registerDefinedVariable(engine, variableMap, definedVariables()[0]);

  engine.makeInvariant<ElementVar>(variableMap.at(_b), as, outputId);
}

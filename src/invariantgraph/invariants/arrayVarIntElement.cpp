#include "../parseHelper.hpp"
#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"
#include "invariants/elementVar.hpp"

std::unique_ptr<invariantgraph::ArrayVarIntElementNode>
invariantgraph::ArrayVarIntElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_var_int_element");
  assert(constraint.arguments.size() == 3);

  auto b = mappedVariable(constraint.arguments[0], variableMap);
  auto as = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::ArrayVarIntElementNode>(b, as, c);
}

void invariantgraph::ArrayVarIntElementNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> as;
  std::transform(dynamicInputs().begin(), dynamicInputs().end(),
                 std::back_inserter(as),
                 [&](auto var) { return variableMap.at(var); });

  engine.makeInvariant<ElementVar>(variableMap.at(b()), as,
                                   getOutputVarId(engine, variableMap, _output));

  InvariantNode::registerWithEngine(engine, variableMap);
}

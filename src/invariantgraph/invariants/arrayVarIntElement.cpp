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

void invariantgraph::ArrayVarIntElementNode::createDefinedVariables(
    Engine& engine) {
  // TODO: offset can be different than 1
  registerDefinedVariable(engine, definedVariables().front(), 1);
}

void invariantgraph::ArrayVarIntElementNode::registerWithEngine(
    Engine& engine) {
  std::vector<VarId> as;
  std::transform(dynamicInputs().begin(), dynamicInputs().end(),
                 std::back_inserter(as),
                 [&](auto node) { return node->varId(); });

  assert(definedVariables().front()->varId() != NULL_ID);

  engine.makeInvariant<ElementVar>(b()->varId(), as,
                                   definedVariables().front()->varId());
}

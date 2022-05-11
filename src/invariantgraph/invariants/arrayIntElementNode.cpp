#include "invariantgraph/invariants/arrayIntElementNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/elementConst.hpp"

std::unique_ptr<invariantgraph::ArrayIntElementNode>
invariantgraph::ArrayIntElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_int_element");
  assert(constraint.arguments.size() == 3);

  auto as = integerVector(model, constraint.arguments[1]);
  auto b = mappedVariable(constraint.arguments[0], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::ArrayIntElementNode>(as, b, c);
}

void invariantgraph::ArrayIntElementNode::createDefinedVariables(
    Engine& engine) {
  // TODO: offset can be different than 1
  registerDefinedVariable(engine, definedVariables().front(), 1);
}

void invariantgraph::ArrayIntElementNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);

  engine.makeInvariant<ElementConst>(b()->varId(), _as,
                                     definedVariables().front()->varId());
}

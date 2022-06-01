#include "invariantgraph/invariants/arrayIntElementNode.hpp"

#include "../parseHelper.hpp"
#include "views/elementConst.hpp"

std::unique_ptr<invariantgraph::ArrayIntElementNode>
invariantgraph::ArrayIntElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto as = integerVector(model, constraint.arguments[1]);
  auto b = mappedVariable(constraint.arguments[0], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::ArrayIntElementNode>(as, b, c);
}

void invariantgraph::ArrayIntElementNode::createDefinedVariables(
    Engine& engine) {
  // TODO: offset can be different than 1
  if (definedVariables().front()->varId() == NULL_ID) {
    assert(b()->varId() != NULL_ID);
    definedVariables().front()->setVarId(
        engine.makeIntView<ElementConst>(b()->varId(), _as));
  }
}

void invariantgraph::ArrayIntElementNode::registerWithEngine(Engine&) {}

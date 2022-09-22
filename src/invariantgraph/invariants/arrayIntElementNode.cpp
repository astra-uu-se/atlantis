#include "invariantgraph/invariants/arrayIntElementNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayIntElementNode>
invariantgraph::ArrayIntElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto as = integerVector(model, constraint.arguments[1]);
  auto idx = mappedVariable(constraint.arguments[0], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);
  const Int offset = constraint.name != "array_int_element_offset"
                         ? 1
                         : idx->domain().lowerBound();

  return std::make_unique<invariantgraph::ArrayIntElementNode>(as, idx, c,
                                                               offset);
}

void invariantgraph::ArrayIntElementNode::createDefinedVariables(
    Engine& engine) {
  if (definedVariables().front()->varId(this) == NULL_ID) {
    assert(b()->inputVarId() != NULL_ID);
    definedVariables().front()->setVarId(
        engine.makeIntView<ElementConst>(b()->inputVarId(), _as, _offset),
        this);
  }
}

void invariantgraph::ArrayIntElementNode::registerWithEngine(Engine&) {}

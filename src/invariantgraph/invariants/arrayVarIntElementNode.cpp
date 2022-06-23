#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayVarIntElementNode>
invariantgraph::ArrayVarIntElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto idx = mappedVariable(constraint.arguments[0], variableMap);
  auto as = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  // Compute offset if nonshifted variant:
  Int offset = constraint.name != "array_var_int_element_offset"
                   ? 1
                   : integerValue(model, constraint.arguments.at(3));

  assert(offset <= idx->domain().lowerBound());
  return std::make_unique<invariantgraph::ArrayVarIntElementNode>(idx, as, c,
                                                                  offset);
}

void invariantgraph::ArrayVarIntElementNode::createDefinedVariables(
    Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front(), _offset);
}

void invariantgraph::ArrayVarIntElementNode::registerWithEngine(
    Engine& engine) {
  std::vector<VarId> as;
  std::transform(dynamicInputs().begin(), dynamicInputs().end(),
                 std::back_inserter(as),
                 [&](auto node) { return node->varId(); });

  assert(definedVariables().front()->varId() != NULL_ID);

  engine.makeInvariant<ElementVar>(definedVariables().front()->varId(),
                                   b()->varId(), as, _offset);
}

#include "invariantgraph/invariants/arrayVarBoolElementNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/elementVar.hpp"

std::unique_ptr<invariantgraph::ArrayVarBoolElementNode>
invariantgraph::ArrayVarBoolElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto b = mappedVariable(constraint.arguments[0], variableMap);

  // Compute offset if nonshifted variant:
  Int offset = constraint.name != "array_var_bool_element_nonshifted"
                   ? 1
                   : b->domain().lowerBound();

  auto as = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::ArrayVarBoolElementNode>(b, as, c,
                                                                   offset);
}

void invariantgraph::ArrayVarBoolElementNode::createDefinedVariables(
    Engine& engine) {
  // TODO: offset can be different than 1
  registerDefinedVariable(engine, definedVariables().front(), 1);
}

void invariantgraph::ArrayVarBoolElementNode::registerWithEngine(
    Engine& engine) {
  std::vector<VarId> as;
  std::transform(dynamicInputs().begin(), dynamicInputs().end(),
                 std::back_inserter(as),
                 [&](auto node) { return node->inputVarId(); });

  assert(definedVariables().front()->varId(this) != NULL_ID);
  engine.makeInvariant<ElementVar>(definedVariables().front()->varId(this),
                                   b()->inputVarId(), as);
}

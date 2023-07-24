#include "invariantgraph/invariants/arrayVarBoolElementNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayVarBoolElementNode>
ArrayVarBoolElementNode::fromModelConstraint(
    const fznparser::Model& model, const fznparser::Constraint& constraint,
    std::unordered_map<std::string_view, VariableNode>& variableMap) {
  //  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto b = mappedVariable(constraint.arguments[0], variableMap);

  // Compute offset if nonshifted variant:
  Int offset = constraint.name != "array_var_bool_element_nonshifted"
                   ? 1
                   : b->domain().lowerBound();

  auto as = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<ArrayVarBoolElementNode>(b, as, c, offset);
}

void ArrayVarBoolElementNode::createDefinedVariables(Engine& engine) {
  // TODO: offset can be different than 1
  registerDefinedVariable(engine, definedVariables().front(), 1);
}

void ArrayVarBoolElementNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> as;
  std::transform(dynamicInputs().begin(), dynamicInputs().end(),
                 std::back_inserter(as),
                 [&](auto node) { return node->varId(); });

  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<ElementVar>(engine, definedVariables().front()->varId(),
                                   b()->varId(), as);
}

}  // namespace invariantgraph
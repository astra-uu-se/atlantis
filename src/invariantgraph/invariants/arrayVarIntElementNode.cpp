#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayVarIntElementNode>
ArrayVarIntElementNode::fromModelConstraint(
    const fznparser::Model& model, const fznparser::Constraint& constraint,
    std::unordered_map<std::string_view, VariableNode>& variableMap) {
  //  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto idx = mappedVariable(constraint.arguments[0], variableMap);
  auto as = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = mappedVariable(constraint.arguments[2], variableMap);

  // Compute offset if nonshifted variant:
  Int offset = constraint.name != "array_var_int_element_nonshifted"
                   ? 1
                   : idx->domain().lowerBound();

  return std::make_unique<ArrayVarIntElementNode>(idx, as, c, offset);
}

void ArrayVarIntElementNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front(), _offset);
}

void ArrayVarIntElementNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> as;
  std::transform(dynamicInputs().begin(), dynamicInputs().end(),
                 std::back_inserter(as),
                 [&](auto node) { return node->varId(); });

  assert(definedVariables().front()->varId() != NULL_ID);

  engine.makeInvariant<ElementVar>(engine, definedVariables().front()->varId(),
                                   b()->varId(), as, _offset);
}

}  // namespace invariantgraph
#include "invariantgraph/invariants/intMinNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntMinNode> IntMinNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    std::unordered_map<std::string_view, VariableNode>& variableMap) {
  //  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<IntMinNode>(a, b, output);
}

void IntMinNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void IntMinNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<BinaryMin>(engine, definedVariables().front()->varId(),
                                  a()->varId(), b()->varId());
}

}  // namespace invariantgraph
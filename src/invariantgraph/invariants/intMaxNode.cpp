#include "invariantgraph/invariants/intMaxNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntMaxNode> IntMaxNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    std::unordered_map<std::string_view, VariableNode>& variableMap) {
  //  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<IntMaxNode>(a, b, output);
}

void IntMaxNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void IntMaxNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<BinaryMax>(engine, definedVariables().front()->varId(),
                                  a()->varId(), b()->varId());
}

}  // namespace invariantgraph
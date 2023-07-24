#include "invariantgraph/invariants/intTimesNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntTimesNode> IntTimesNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    std::unordered_map<std::string_view, VariableNode>& variableMap) {
  //  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto output = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<IntTimesNode>(a, b, output);
}

void IntTimesNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void IntTimesNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<Times>(engine, definedVariables().front()->varId(),
                              a()->varId(), b()->varId());
}

}  // namespace invariantgraph
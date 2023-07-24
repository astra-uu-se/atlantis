#include "invariantgraph/invariants/arrayIntMinimumNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayIntMinimumNode> ArrayIntMinimumNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  InvariantNode* output = invariantGraph.addVariable(
      std::get<IntArg>(constraint.arguments().at(0)));

  std::vector<InvariantNode*> inputs = invariantGraph.addVariableArray(
      std::get<IntVarArray>(constraint.arguments().at(1)));

  return std::make_unique<ArrayIntMinimumNode>(inputs, output);
}

void ArrayIntMinimumNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void ArrayIntMinimumNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return node->varId(); });

  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<MinSparse>(engine, definedVariables().front()->varId(),
                                  variables);
}

}  // namespace invariantgraph
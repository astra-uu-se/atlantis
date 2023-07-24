#include "invariantgraph/invariants/arrayIntMaximumNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayIntMaximumNode> ArrayIntMaximumNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  InvariantNode* output = invariantGraph.addVariable(
      std::get<IntArg>(constraint.arguments().at(0)));

  std::vector<InvariantNode*> inputs = invariantGraph.addVariableArray(
      std::get<IntVarArray>(constraint.arguments().at(1)));

  return std::make_unique<ArrayIntMaximumNode>(inputs, output);
}

void ArrayIntMaximumNode::createDefinedVariables(Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void ArrayIntMaximumNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return node->varId(); });

  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<MaxSparse>(engine, definedVariables().front()->varId(),
                                  variables);
}

}  // namespace invariantgraph
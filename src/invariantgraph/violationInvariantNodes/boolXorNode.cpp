#include "invariantgraph/violationInvariantNodes/boolXorNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<BoolXorNode> BoolXorNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolXor constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolXor constraint takes two var bool arguments");
    }
  }

  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolXorNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    return std::make_unique<BoolXorNode>(a, b, reified.toParameter());
  }
  return std::make_unique<BoolXorNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void BoolXorNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                          Engine& engine) {
  registerViolation(engine);
}

void BoolXorNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId() != NULL_ID);

  if (shouldHold()) {
    engine.makeInvariant<BoolXor>(engine, violationVarId(), a()->varId(),
                                  b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<BoolEqual>(engine, violationVarId(), a()->varId(),
                                     b()->varId());
  }
}

}  // namespace invariantgraph
#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b,
                       VarNodeId r = VarNodeId(NULL_NODE_ID))
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, bool shouldHold = true)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

BoolEqNode::BoolEqNode(InvariantGraph invariantGraph, VarNodeId a, VarNodeId b,
                       bool shouldHold = true)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {
  init(invariantGraph, invariantGraph.nextInvariantNodeId());
}

std::unique_ptr<BoolEqNode> BoolEqNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolEq constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolEq constraint takes two var bool arguments");
    }
  }

  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolEqNode>(a, b, true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<invariantgraph::BoolEqNode>(a, b,
                                                        reified.toParameter());
  }
  return std::make_unique<invariantgraph::BoolEqNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void BoolEqNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                         Engine& engine) {
  registerViolation(engine);
}

void BoolEqNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<BoolEqual>(engine, violationVarId(), a()->varId(),
                                     b()->varId());
  } else {
    engine.makeInvariant<BoolXor>(engine, violationVarId(), a()->varId(),
                                  b()->varId());
  }
}

}  // namespace invariantgraph
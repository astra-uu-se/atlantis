#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

std::unique_ptr<BoolEqNode> BoolEqNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
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
                                         propagation::Engine& engine) {
  registerViolation(invariantGraph, engine);
}

void BoolEqNode::registerNode(InvariantGraph& invariantGraph, propagation::Engine& engine) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(invariantGraph.varId(a()) != propagation::NULL_ID);
  assert(invariantGraph.varId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<propagation::BoolEqual>(engine, violationVarId(invariantGraph),
                                     invariantGraph.varId(a()),
                                     invariantGraph.varId(b()));
  } else {
    engine.makeInvariant<propagation::BoolXor>(engine, violationVarId(invariantGraph),
                                  invariantGraph.varId(a()),
                                  invariantGraph.varId(b()));
  }
}

}  // namespace invariantgraph
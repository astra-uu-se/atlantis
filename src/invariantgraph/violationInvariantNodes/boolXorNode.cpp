#include "invariantgraph/violationInvariantNodes/boolXorNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolXorNode::BoolXorNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}
BoolXorNode::BoolXorNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

std::unique_ptr<BoolXorNode> BoolXorNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
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

void BoolXorNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void BoolXorNode::registerNode(InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeInvariant<propagation::BoolXor>(solver, violationVarId(invariantGraph),
                                  invariantGraph.varId(a()),
                                  invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeConstraint<propagation::BoolEqual>(solver, violationVarId(invariantGraph),
                                     invariantGraph.varId(a()),
                                     invariantGraph.varId(b()));
  }
}

}  // namespace invariantgraph
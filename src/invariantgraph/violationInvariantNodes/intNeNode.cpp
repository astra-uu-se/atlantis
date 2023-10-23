#include "invariantgraph/violationInvariantNodes/intNeNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntNeNode::IntNeNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode({a, b}, r) {}
IntNeNode::IntNeNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode({a, b}, shouldHold) {}

std::unique_ptr<IntNeNode> IntNeNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
      constraint.arguments().size() != 3) {
    throw std::runtime_error("IntNe constraint takes two var bool arguments");
  }

  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<IntNeNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    return std::make_unique<IntNeNode>(a, b, reified.toParameter());
  }
  return std::make_unique<IntNeNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void IntNeNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void IntNeNode::registerNode(InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeConstraint<propagation::NotEqual>(solver, violationVarId(invariantGraph),
                                    invariantGraph.varId(a()),
                                    invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeConstraint<propagation::Equal>(solver, violationVarId(invariantGraph),
                                 invariantGraph.varId(a()),
                                 invariantGraph.varId(b()));
  }
}

}  // namespace invariantgraph
#include "invariantgraph/violationInvariantNodes/intLtNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntLtNode::IntLtNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode({a, b}, r) {}
IntLtNode::IntLtNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode({a, b}, shouldHold) {}

std::unique_ptr<IntLtNode> IntLtNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
      constraint.arguments().size() != 3) {
    throw std::runtime_error("IntLe constraint takes two var bool arguments");
  }

  VarNodeId a = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  VarNodeId b = invariantGraph.createVarNode(
      std::get<fznparser::IntArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<IntLtNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    return std::make_unique<IntLtNode>(a, b, reified.toParameter());
  }
  return std::make_unique<IntLtNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void IntLtNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void IntLtNode::registerNode(InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeConstraint<propagation::LessThan>(solver, violationVarId(invariantGraph),
                                    invariantGraph.varId(a()),
                                    invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeConstraint<propagation::LessEqual>(solver, violationVarId(invariantGraph),
                                     invariantGraph.varId(b()),
                                     invariantGraph.varId(a()));
  }
}

}  // namespace invariantgraph
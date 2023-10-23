#include "invariantgraph/violationInvariantNodes/intLeNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntLeNode::IntLeNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode({a, b}, r) {}

IntLeNode::IntLeNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode({a, b}, shouldHold) {}

std::unique_ptr<IntLeNode> IntLeNode::fromModelConstraint(
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
    return std::make_unique<IntLeNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    return std::make_unique<IntLeNode>(a, b, reified.toParameter());
  }
  return std::make_unique<IntLeNode>(
      a, b, invariantGraph.createVarNode(reified.var()));
}

void IntLeNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void IntLeNode::registerNode(InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeConstraint<propagation::LessEqual>(solver, violationVarId(invariantGraph),
                                     invariantGraph.varId(a()),
                                     invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeConstraint<propagation::LessThan>(solver, violationVarId(invariantGraph),
                                    invariantGraph.varId(b()),
                                    invariantGraph.varId(a()));
  }
}

}  // namespace invariantgraph
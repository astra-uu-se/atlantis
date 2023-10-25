#include "invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityLowUpNode::GlobalCardinalityLowUpNode(
    std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, VarNodeId r)
    : ViolationInvariantNode({}, std::move(x), r),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

GlobalCardinalityLowUpNode::GlobalCardinalityLowUpNode(
    std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, bool shouldHold)
    : ViolationInvariantNode({}, std::move(x), shouldHold),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

std::unique_ptr<GlobalCardinalityLowUpNode>
GlobalCardinalityLowUpNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<VarNodeId> inputs = invariantGraph.createVarNodes(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)));

  std::vector<Int> cover =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();

  std::vector<Int> low =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2))
          .toParVector();

  std::vector<Int> up =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(3))
          .toParVector();

  assert(cover.size() == low.size());
  assert(cover.size() == up.size());

  bool shouldHold = true;
  VarNodeId r = NULL_NODE_ID;

  if (constraint.arguments().size() == 5) {
    const fznparser::BoolArg reified =
        std::get<fznparser::BoolArg>(constraint.arguments().at(4));
    if (reified.isFixed()) {
      shouldHold = reified.toParameter();
    } else {
      r = invariantGraph.createVarNode(reified.var());
    }
  }

  if (r != NULL_NODE_ID) {
    return std::make_unique<GlobalCardinalityLowUpNode>(
        std::move(inputs), std::move(cover), std::move(low), std::move(up), r);
  }
  assert(r == NULL_NODE_ID);
  return std::make_unique<GlobalCardinalityLowUpNode>(
      std::move(inputs), std::move(cover), std::move(low), std::move(up),
      shouldHold);
}

void GlobalCardinalityLowUpNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    if (!shouldHold()) {
      _intermediate = solver.makeIntVar(0, 0, staticInputVarNodeIds().size());
      setViolationVarId(invariantGraph, solver.makeIntView<propagation::NotEqualConst>(
                                            solver, _intermediate, 0));
    } else {
      registerViolation(invariantGraph, solver);
    }
  }
}

void GlobalCardinalityLowUpNode::registerNode(InvariantGraph& invariantGraph,
                                              propagation::SolverBase& solver) {
  std::vector<propagation::VarId> inputs;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputs),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  if (shouldHold()) {
    solver.makeInvariant<propagation::GlobalCardinalityConst<false>>(
        solver, violationVarId(invariantGraph), inputs, _cover, _low, _up);
  } else {
    solver.makeInvariant<propagation::GlobalCardinalityConst<false>>(
        solver, _intermediate, inputs, _cover, _low, _up);
  }
}

}  // namespace invariantgraph
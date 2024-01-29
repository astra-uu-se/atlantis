#include "invariantgraph/violationInvariantNodes/globalCardinalityLowUpClosedNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityLowUpClosedNode::GlobalCardinalityLowUpClosedNode(

    std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, VarNodeId r)
    : ViolationInvariantNode({}, std::move(x), r),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

GlobalCardinalityLowUpClosedNode::GlobalCardinalityLowUpClosedNode(
    std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, bool shouldHold)
    : ViolationInvariantNode({}, std::move(x), shouldHold),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

void GlobalCardinalityLowUpClosedNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    if (!shouldHold()) {
      _intermediate = solver.makeIntVar(0, 0, staticInputVarNodeIds().size());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 0));
    } else {
      registerViolation(invariantGraph, solver);
    }
  }
}

void GlobalCardinalityLowUpClosedNode::registerNode(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  std::vector<propagation::VarId> inputs;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputs),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  if (shouldHold()) {
    solver.makeInvariant<propagation::GlobalCardinalityConst<true>>(
        solver, violationVarId(invariantGraph), inputs, _cover, _low, _up);
  } else {
    solver.makeInvariant<propagation::GlobalCardinalityConst<true>>(
        solver, _intermediate, inputs, _cover, _low, _up);
  }
}

}  // namespace atlantis::invariantgraph
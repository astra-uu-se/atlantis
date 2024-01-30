#include "invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    std::vector<VarNodeId>&& inputs, std::vector<Int>&& cover,
    std::vector<VarNodeId>&& counts, VarNodeId r)
    : ViolationInvariantNode(std::move(counts), std::move(inputs), r),
      _cover(std::move(cover)) {}

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    std::vector<VarNodeId>&& inputs, std::vector<Int>&& cover,
    std::vector<VarNodeId>&& counts, bool shouldHold)
    : ViolationInvariantNode(std::move(counts), std::move(inputs), shouldHold),
      _cover(std::move(cover)) {}

void GlobalCardinalityClosedNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) != propagation::NULL_ID) {
    return;
  }
  if (shouldHold()) {
    registerViolation(invariantGraph, solver);
  } else {
    assert(!isReified());
    _intermediate = solver.makeIntVar(0, 0, 0);
    setViolationVarId(invariantGraph,
                      solver.makeIntView<propagation::NotEqualConst>(
                          solver, _intermediate, 0));
  }
}

void GlobalCardinalityClosedNode::registerNode(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](VarNodeId id) { return invariantGraph.varId(id); });

  std::vector<propagation::VarId> outputVarIds;
  std::transform(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                 std::back_inserter(outputVarIds),
                 [&](VarNodeId id) { return invariantGraph.varId(id); });

  assert(_intermediate == isReified() && shouldHold());
  solver.makeInvariant<propagation::GlobalCardinalityClosed>(
      solver,
      _intermediate != propagation::NULL_ID ? violationVarId(invariantGraph)
                                            : _intermediate,
      std::move(outputVarIds), std::move(inputVarIds), std::vector<Int>(_cover));
}

}  // namespace atlantis::invariantgraph
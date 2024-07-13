#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/exists.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/globalCardinalityLowUp.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

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

void GlobalCardinalityLowUpNode::init(InvariantGraph& graph,
                                      const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(outputVarNodeIds().begin() + 1, outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void GlobalCardinalityLowUpNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    if (!shouldHold()) {
      _intermediate = solver.makeIntVar(
          0, 0, static_cast<Int>(staticInputVarNodeIds().size()));
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 0));
    } else {
      registerViolation(invariantGraph, solver);
    }
  }
}

void GlobalCardinalityLowUpNode::registerNode(InvariantGraph& invariantGraph,
                                              propagation::SolverBase& solver) {
  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  if (shouldHold()) {
    solver.makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver, violationVarId(invariantGraph), std::move(inputVarIds),
        std::vector<Int>(_cover), std::vector<Int>(_low),
        std::vector<Int>(_up));
  } else {
    solver.makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver, _intermediate, std::move(inputVarIds), std::vector<Int>(_cover),
        std::vector<Int>(_low), std::vector<Int>(_up));
  }
}

}  // namespace atlantis::invariantgraph

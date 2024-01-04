#include "invariantgraph/invariantNodes/intCountNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntCountNode::IntCountNode(std::vector<VarNodeId>&& vars, VarNodeId needle,
                           VarNodeId count)
    : InvariantNode(append(vars, needle), std::move(vars)) {}

std::vector<VarNodeId> haystack() const {
  std::vector<VarNodeId> inputs;
  inputs.reserve(staticInputVarNodeIds().size() - 1);
  std::copy(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end() - 1,
            std::back_inserter(inputs));
  return inputs;
}

VarNodeId needle() const { return staticInputVarNodeIds().back(); }

void IntCountNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntCountNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<VarNodeId> h = haystack();
  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(h.size());

  std::transform(
      h.begin(), h.end(), std::back_inserter(solverVars),
      [&](const VarNodeId node) { return invariantGraph.varId(node); });

  solver.makeInvariant<propagation::Count>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      needle().varId(), solverVars);
}

}  // namespace atlantis::invariantgraph
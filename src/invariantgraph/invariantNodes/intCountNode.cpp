#include "invariantgraph/invariantNodes/intCountNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntCountNode::IntCountNode(std::vector<VarNodeId>&& vars, Int needle,
                           VarNodeId count)
    : InvariantNode(std::vector<VarNodeId>{count}, std::move(vars)),
      _needle(needle) {}

const std::vector<VarNodeId>& IntCountNode::haystack() const {
  return staticInputVarNodeIds();
}

Int IntCountNode::needle() const { return _needle; }

void IntCountNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntCountNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());

  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars), [&](const VarNodeId node) {
                   return invariantGraph.varId(node);
                 });

  solver.makeInvariant<propagation::CountConst>(
      solver, invariantGraph.varId(outputVarNodeIds().front()), needle(),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
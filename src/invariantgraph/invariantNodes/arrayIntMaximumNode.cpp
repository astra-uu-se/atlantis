#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/maxSparse.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)) {}

void ArrayIntMaximumNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

bool ArrayIntMaximumNode::canBeReplaced(
    const InvariantGraph& invariantGraph) const {
  size_t numFixed = 0;
  for (const auto& input : staticInputVarNodeIds()) {
    numFixed +=
        static_cast<size_t>(invariantGraph.varNodeConst(input).isFixed());
    if (numFixed > 1) {
      return false;
    }
  }
  return true;
}

bool ArrayIntMaximumNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  assert(staticInputVarNodeIds().size() == 1);
  invariantGraph.replaceVarNode(staticInputVarNodeIds().front(),
                                outputVarNodeIds().front());
  deactivate(invariantGraph);
  return true;
}

void ArrayIntMaximumNode::registerNode(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::MaxSparse>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph

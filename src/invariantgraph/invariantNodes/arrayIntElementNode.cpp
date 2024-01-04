#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayIntElementNode::ArrayIntElementNode(std::vector<Int>&& as, VarNodeId b,
                                         VarNodeId output, Int offset)
    : InvariantNode({output}, {b}), _as(std::move(as)), _offset(offset) {}

void ArrayIntElementNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    assert(invariantGraph.varId(b()) != propagation::NULL_ID);
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::ElementConst>(
            solver, invariantGraph.varId(b()), _as, _offset));
  }
}

void ArrayIntElementNode::registerNode(InvariantGraph&,
                                       propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
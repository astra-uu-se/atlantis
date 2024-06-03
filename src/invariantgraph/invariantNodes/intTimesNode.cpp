#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/times.hpp"

namespace atlantis::invariantgraph {

IntTimesNode::IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

void IntTimesNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntTimesNode::propagate(InvariantGraph& graph) {
  for (VarNodeId input : std::array<VarNodeId, 2>{a(), b()}) {
    if (graph.isFixed(input) && graph.lowerBound(input) == 0) {
      graph.fixToValue(outputVarNodeIds().front(), 0);
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  if (graph.isFixed(a()) && graph.isFixed(b())) {
    Int result = graph.lowerBound(a()) * graph.lowerBound(b());
    graph.fixToValue(outputVarNodeIds().front(), result);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  if (graph.isFixed(outputVarNodeIds().front())) {
    for (VarNodeId input : std::array<VarNodeId, 2>{a(), b()}) {
      if (graph.isFixed(input)) {
        assert(graph.lowerBound(input) != 0);
        if (graph.lowerBound(outputVarNodeIds().front()) %
                graph.lowerBound(input) !=
            0) {
          setState(InvariantNodeState::INFEASIBLE);
          return;
        }
        const Int result = graph.lowerBound(outputVarNodeIds().front()) /
                           graph.lowerBound(input);
        graph.fixToValue(outputVarNodeIds().front(), result);
        setState(InvariantNodeState::SUBSUMED);
      }
    }
  }
}

void IntTimesNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Times>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

VarNodeId IntTimesNode::a() const { return staticInputVarNodeIds().front(); }
VarNodeId IntTimesNode::b() const { return staticInputVarNodeIds().back(); }

}  // namespace atlantis::invariantgraph

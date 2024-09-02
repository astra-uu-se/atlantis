#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

#include <cmath>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/pow.hpp"

namespace atlantis::invariantgraph {

IntPowNode::IntPowNode(VarNodeId base, VarNodeId exponent, VarNodeId power)
    : InvariantNode({power}, {base, exponent}) {}

void IntPowNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void IntPowNode::registerOutputVars(InvariantGraph& graph,
                                    propagation::SolverBase& solver) {
  makeSolverVar(solver, graph.varNode(outputVarNodeIds().front()));
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntPowNode::registerNode(InvariantGraph& graph,
                              propagation::SolverBase& solver) {
  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);
  solver.makeInvariant<propagation::Pow>(
      solver, graph.varId(outputVarNodeIds().front()), graph.varId(base()),
      graph.varId(exponent()));
}

VarNodeId IntPowNode::base() const { return staticInputVarNodeIds().front(); }
VarNodeId IntPowNode::exponent() const {
  return staticInputVarNodeIds().back();
}
VarNodeId IntPowNode::power() const { return outputVarNodeIds().front(); }

std::string IntPowNode::dotLangIdentifier() const { return "int_pow"; }

}  // namespace atlantis::invariantgraph

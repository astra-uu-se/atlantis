#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/mod.hpp"

namespace atlantis::invariantgraph {

IntModNode::IntModNode(VarNodeId numerator, VarNodeId denominator,
                       VarNodeId remainder)
    : InvariantNode({remainder}, {numerator, denominator}) {}

void IntModNode::init(const InvariantNodeId& id) {
  InvariantNode::init(id);
  assert(graph.varNodeConst(remainder()).isIntVar());
  assert(graph.varNodeConst(numerator()).isIntVar());
  assert(graph.varNodeConst(denominator()).isIntVar());
}

void IntModNode::registerOutputVars() {
  makeSolverVar(solver, graph.varNode(outputVarNodeIds().front()));
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntModNode::registerNode() {
  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);
  solver.makeInvariant<propagation::Mod>(
      solver, graph.varId(outputVarNodeIds().front()), graph.varId(numerator()),
      graph.varId(denominator()));
}

VarNodeId IntModNode::numerator() const {
  return staticInputVarNodeIds().front();
}
VarNodeId IntModNode::denominator() const {
  return staticInputVarNodeIds().back();
}
VarNodeId IntModNode::remainder() const { return outputVarNodeIds().front(); }

}  // namespace atlantis::invariantgraph

#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

#include <cmath>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/pow.hpp"

namespace atlantis::invariantgraph {

IntPowNode::IntPowNode(IInvariantGraph& graph, VarNodeId base,
                       VarNodeId exponent, VarNodeId power)
    : InvariantNode(graph, {power}, {base, exponent}) {}

void IntPowNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void IntPowNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front());
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntPowNode::registerNode() {
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  solver().makeInvariant<propagation::Pow>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(base()), invariantGraph().varId(exponent()));
}

VarNodeId IntPowNode::base() const { return staticInputVarNodeIds().front(); }
VarNodeId IntPowNode::exponent() const {
  return staticInputVarNodeIds().back();
}
VarNodeId IntPowNode::power() const { return outputVarNodeIds().front(); }

std::string IntPowNode::dotLangIdentifier() const { return "int_pow"; }

}  // namespace atlantis::invariantgraph

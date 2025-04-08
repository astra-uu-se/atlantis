#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/pow.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

IntPowNode::IntPowNode(InvariantGraph& graph, VarNodeId base,
                       VarNodeId exponent, VarNodeId power)
    : InvariantNode(graph, {power}, {base, exponent}) {}

void IntPowNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void IntPowNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front());
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
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

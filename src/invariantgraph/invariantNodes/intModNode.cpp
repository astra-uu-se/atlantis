#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/mod.hpp"

namespace atlantis::invariantgraph {

IntModNode::IntModNode(IInvariantGraph& graph, VarNodeId numerator,
                       VarNodeId denominator, VarNodeId remainder)
    : InvariantNode(graph, {remainder}, {numerator, denominator}) {}

void IntModNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst().varNodeConst(remainder()).isIntVar());
  assert(invariantGraphConst().varNodeConst(numerator()).isIntVar());
  assert(invariantGraphConst().varNodeConst(denominator()).isIntVar());
}

void IntModNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front());
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntModNode::registerNode() {
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  solver().makeInvariant<propagation::Mod>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(numerator()),
      invariantGraph().varId(denominator()));
}

VarNodeId IntModNode::numerator() const {
  return staticInputVarNodeIds().front();
}
VarNodeId IntModNode::denominator() const {
  return staticInputVarNodeIds().back();
}
VarNodeId IntModNode::remainder() const { return outputVarNodeIds().front(); }

std::string IntModNode::dotLangIdentifier() const { return "int_mod"; }

}  // namespace atlantis::invariantgraph

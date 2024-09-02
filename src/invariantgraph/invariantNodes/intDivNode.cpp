#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/intDiv.hpp"

namespace atlantis::invariantgraph {

IntDivNode::IntDivNode(IInvariantGraph& graph, VarNodeId numerator,
                       VarNodeId denominator, VarNodeId quotient)
    : InvariantNode(graph, {quotient}, {numerator, denominator}) {}

void IntDivNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst().varNodeConst(quotient()).isIntVar());
  assert(invariantGraphConst().varNodeConst(numerator()).isIntVar());
  assert(invariantGraphConst().varNodeConst(denominator()).isIntVar());
}

bool IntDivNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         invariantGraphConst().varNodeConst(denominator()).isFixed() &&
         invariantGraphConst().varNodeConst(denominator()).lowerBound() == 1;
}

bool IntDivNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(invariantGraph().varNode(denominator()).isFixed() &&
         invariantGraph().varNode(denominator()).lowerBound() == 1);
  invariantGraph().replaceVarNode(quotient(), numerator());
  return true;
}

void invariantgraph::IntDivNode::registerOutputVars() {
  makeSolverVar(quotient());
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void invariantgraph::IntDivNode::registerNode() {
  assert(invariantGraph().varId(quotient()) != propagation::NULL_ID);
  assert(invariantGraph().varId(quotient()).isVar());

  solver().makeInvariant<propagation::IntDiv>(
      solver(), invariantGraph().varId(quotient()),
      invariantGraph().varId(numerator()),
      invariantGraph().varId(denominator()));
}

VarNodeId IntDivNode::numerator() const noexcept {
  return staticInputVarNodeIds().front();
}
VarNodeId IntDivNode::denominator() const noexcept {
  return staticInputVarNodeIds().back();
}
VarNodeId IntDivNode::quotient() const noexcept {
  return outputVarNodeIds().front();
}

std::string IntDivNode::dotLangIdentifier() const { return "int_div"; }

}  // namespace atlantis::invariantgraph

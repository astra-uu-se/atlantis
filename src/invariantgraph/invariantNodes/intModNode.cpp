#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/intModViewNode.hpp"
#include "atlantis/propagation/invariants/mod.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

IntModNode::IntModNode(InvariantGraph& graph, VarNodeId numerator,
                       VarNodeId denominator, VarNodeId remainder)
    : InvariantNode(graph, {remainder}, {numerator, denominator}) {}

void IntModNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst().varNodeConst(remainder()).isIntVar());
  assert(invariantGraphConst().varNodeConst(numerator()).isIntVar());
  assert(invariantGraphConst().varNodeConst(denominator()).isIntVar());
}

void IntModNode::updateState() {
  auto& dNode = invariantGraph().varNode(denominator());
  dNode.removeValue(Int{0});

  const auto& nNode = invariantGraphConst().varNodeConst(numerator());
  auto& rNode = invariantGraph().varNode(remainder());

  if (nNode.isFixed() && nNode.lowerBound() == 0) {
    rNode.fixToValue(Int{0});
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  if (nNode.isFixed() && nNode.isFixed() == 0) {
    rNode.fixToValue(nNode.lowerBound() % std::abs(nNode.upperBound()));
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  if (nNode.lowerBound() >= 0) {
    rNode.removeValuesBelow(0);
  }
  if (nNode.upperBound() <= 0) {
    rNode.removeValuesAbove(0);
  }

  const Int lb = std::min(dNode.lowerBound(), -dNode.upperBound()) + 1;
  const Int ub = std::max(dNode.upperBound(), -dNode.lowerBound()) - 1;
  rNode.removeValuesBelow(lb);
  rNode.removeValuesAbove(ub);
}

bool IntModNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         invariantGraphConst().varNodeConst(denominator()).isFixed();
}

bool IntModNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(invariantGraphConst().varNodeConst(denominator()).isFixed());
  invariantGraph().addInvariantNode(std::make_shared<IntModViewNode>(
      invariantGraph(), numerator(), remainder(),
      invariantGraphConst().varNodeConst(denominator()).lowerBound()));
  return true;
}

void IntModNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front());
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
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

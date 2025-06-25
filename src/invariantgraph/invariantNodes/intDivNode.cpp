#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

#include <stack>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/intDiv.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

IntDivNode::IntDivNode(InvariantGraph& graph, VarNodeId numerator,
                       VarNodeId denominator, VarNodeId quotient)
    : InvariantNode(graph, {quotient}, {numerator, denominator}) {}

void IntDivNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst().varNodeConst(quotient()).isIntVar());
  assert(invariantGraphConst().varNodeConst(numerator()).isIntVar());
  assert(invariantGraphConst().varNodeConst(denominator()).isIntVar());
}

void IntDivNode::updateState() {
  auto& dNode = invariantGraph().varNode(denominator());
  dNode.removeValue(Int{0});

  auto& nNode = invariantGraph().varNode(numerator());
  auto& qNode = invariantGraph().varNode(quotient());

  auto onStack = std::array{true, true, true};
  std::stack<size_t> stack;
  for (size_t i = 0; i < 3; ++i) {
    stack.emplace(i);
  }

  while (!stack.empty()) {
    const size_t index = stack.top();
    stack.pop();
    if (index == 0) {
      // numerator
      const auto arr = std::array{
        qNode.lowerBound() * dNode.lowerBound(),
        qNode.lowerBound() * dNode.upperBound(),
        qNode.upperBound() * dNode.lowerBound(),
        qNode.upperBound() * dNode.upperBound()};
      const Int pLb = nNode.lowerBound();
      const Int pUb = nNode.upperBound();
      nNode.removeValuesBelow(std::ranges::min(arr));
      nNode.removeValuesAbove(std::ranges::max(arr));
      if (pLb != nNode.lowerBound() || pUb != nNode.upperBound()) {
        if (!onStack[1]) {
          stack.push(1);
          onStack[1] = true;
        }
        if (!onStack[2]) {
          stack.push(2);
          onStack[2] = true;
        }
      }
    } else if (index == 1) {
      // denominator
      if (qNode.isFixed() && qNode.lowerBound() == 0) {
        nNode.fixToValue(Int{0});
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      auto arr = std::array{
        nNode.lowerBound() / (qNode.lowerBound() != 0 ? qNode.lowerBound() : qNode.domain()->at(1)),
        nNode.lowerBound() / (qNode.upperBound() != 0 ? qNode.upperBound() : qNode.domain()->at(qNode.domain()->size() - 2)),
        nNode.upperBound() / (qNode.lowerBound() != 0 ? qNode.lowerBound() : qNode.domain()->at(1)),
        nNode.upperBound() / (qNode.upperBound() != 0 ? qNode.upperBound() : qNode.domain()->at(qNode.domain()->size() - 2)),
      };
      const Int pLb = dNode.lowerBound();
      const Int pUb = dNode.upperBound();
      dNode.removeValuesBelow(std::ranges::min(arr));
      dNode.removeValuesAbove(std::ranges::max(arr));
      if (pLb != dNode.lowerBound() || pUb != dNode.upperBound()) {
        if (!onStack[0]) {
          stack.push(0);
          onStack[0] = true;
        }
        if (!onStack[2]) {
          stack.push(2);
          onStack[2] = true;
        }
      }
    } else if (index == 2) {
      // quotient
      assert(!dNode.inDomain(Int{0}));
      const auto arr = std::array{
        nNode.lowerBound() / dNode.lowerBound(),
        nNode.lowerBound() / dNode.upperBound(),
        nNode.upperBound() / dNode.lowerBound(),
        nNode.upperBound() / dNode.upperBound()};
      const Int pLb = qNode.lowerBound();
      const Int pUb = qNode.upperBound();
      qNode.removeValuesBelow(std::ranges::min(arr));
      qNode.removeValuesAbove(std::ranges::max(arr));
      if (pLb != qNode.lowerBound() || pUb != qNode.upperBound()) {
        if (!onStack[0]) {
          stack.push(0);
          onStack[0] = true;
        }
        if (!onStack[1]) {
          stack.push(1);
          onStack[1] = true;
        }
      }
    }
    onStack[index] = false;
  }
  if ((nNode.isFixed() && nNode.lowerBound() == 0) || (qNode.isFixed() && qNode.lowerBound() == 0)) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (nNode.isFixed() && dNode.isFixed() && qNode.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
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

void IntDivNode::registerOutputVars() {
  makeSolverVar(quotient());
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void IntDivNode::registerNode() {
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

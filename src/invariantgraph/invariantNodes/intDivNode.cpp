#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

#include <limits>
#include <stack>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/intAbsNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLtNode.hpp"
#include "atlantis/propagation/invariants/intDiv.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

static Int div_ceil(Int n, Int d) { return n / d + (n % d > 0 ? 1 : 0); }

static Int div_floor(Int n, Int d) { return n / d - (n % d < 0 ? 1 : 0); }

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
      const auto arr =
          std::array{std::pair{qNode.lowerBound(), dNode.lowerBound()},
                     std::pair{qNode.lowerBound(), dNode.upperBound()},
                     std::pair{qNode.upperBound(), dNode.lowerBound()},
                     std::pair{qNode.upperBound(), dNode.upperBound()}};

      const bool zeroInQ = qNode.inDomain(Int{0});

      Int newLb = zeroInQ
                      ? (std::min(dNode.lowerBound(), -dNode.upperBound()) + 1)
                      : std::numeric_limits<Int>::max();
      Int newUb = zeroInQ
                      ? (std::max(-dNode.lowerBound(), dNode.upperBound()) - 1)
                      : std::numeric_limits<Int>::min();
      for (const auto& [q, d] : arr) {
        Int prod;
        if (__builtin_smull_overflow(q, d, &prod)) {
          if ((q >= 0) == (d >= 0)) {
            newUb = std::numeric_limits<Int>::max();
          } else {
            newLb = std::numeric_limits<Int>::min();
          }
          continue;
        }
        newLb = std::min(newLb, prod);
        newUb = std::max(newUb, prod);
      }

      const Int pLb = nNode.lowerBound();
      const Int pUb = nNode.upperBound();
      nNode.removeValuesBelow(newLb);
      nNode.removeValuesAbove(newUb);
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
    } else if (index == 1 && !(qNode.isFixed() && qNode.lowerBound() == 0)) {
      // denominator
      // the case where the quotient is fixed to 0 is handled below and in the
      // replace method.
      const auto arr = std::array{
          std::pair{nNode.lowerBound(), qNode.lowerBound() != 0
                                            ? qNode.lowerBound()
                                            : qNode.domain()->at(1)},
          std::pair{nNode.lowerBound(),
                    qNode.upperBound() != 0
                        ? qNode.upperBound()
                        : qNode.domain()->at(qNode.domain()->size() - 2)},
          std::pair{nNode.upperBound(), qNode.lowerBound() != 0
                                            ? qNode.lowerBound()
                                            : qNode.domain()->at(1)},
          std::pair{nNode.upperBound(),
                    qNode.upperBound() != 0
                        ? qNode.upperBound()
                        : qNode.domain()->at(qNode.domain()->size() - 2)}};
      Int newLb = std::numeric_limits<Int>::max();
      Int newUb = std::numeric_limits<Int>::min();
      for (const auto& [n, d] : arr) {
        newLb = std::ranges::min(
            std::array{newLb, div_floor(n, d), div_ceil(n, d)});
        newUb = std::ranges::max(
            std::array{newUb, div_floor(n, d), div_ceil(n, d)});
      }
      const Int pLb = dNode.lowerBound();
      const Int pUb = dNode.upperBound();
      dNode.removeValuesBelow(newLb);
      dNode.removeValuesAbove(newUb);
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
      const auto arr =
          std::array{std::pair{nNode.lowerBound(), dNode.lowerBound()},
                     std::pair{nNode.lowerBound(), dNode.upperBound()},
                     std::pair{nNode.upperBound(), dNode.lowerBound()},
                     std::pair{nNode.upperBound(), dNode.upperBound()}};
      Int newLb = std::numeric_limits<Int>::max();
      Int newUb = std::numeric_limits<Int>::min();
      for (const auto& [n, d] : arr) {
        newLb = std::ranges::min(
            std::array{newLb, div_floor(n, d), div_ceil(n, d)});
        newUb = std::ranges::max(
            std::array{newUb, div_floor(n, d), div_ceil(n, d)});
      }
      const Int pLb = qNode.lowerBound();
      const Int pUb = qNode.upperBound();
      qNode.removeValuesBelow(newLb);
      qNode.removeValuesAbove(newUb);
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
  if (nNode.isFixed() && dNode.isFixed() && qNode.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (nNode.isFixed() && nNode.lowerBound() == 0) {
    assert(qNode.isFixed() && qNode.lowerBound() == 0);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (qNode.isFixed() && qNode.lowerBound() == 0) {
    if (!nNode.isFixed() && dNode.isFixed()) {
      assert(dNode.isFixed());
      const Int dVal = dNode.lowerBound();
      nNode.removeValuesBelow((dVal > 0 ? -dVal : dVal) + 1);
      nNode.removeValuesAbove((dVal < 0 ? -dVal : dVal) - 1);
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    if (!dNode.isFixed() && nNode.isFixed()) {
      const Int nVal = nNode.lowerBound();
      const Int lb = std::min(-nVal, nVal) + 1;
      const Int ub = std::max(-nVal, nVal) - 1;
      dNode.domain()->intersect(lb, ub - 1);
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
}

bool IntDivNode::canBeReplaced() const {
  const auto& nNode = invariantGraphConst().varNodeConst(numerator());
  const auto& dNode = invariantGraphConst().varNodeConst(denominator());
  const auto& qNode = invariantGraphConst().varNodeConst(quotient());
  return state() == InvariantNodeState::ACTIVE &&
         ((dNode.isFixed() && dNode.lowerBound() == 1) ||
          ((!nNode.isFixed() || !dNode.isFixed()) && qNode.isFixed() &&
           qNode.lowerBound() == 0));
}

bool IntDivNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  auto& dNode = invariantGraph().varNode(denominator());
  if (dNode.isFixed() && dNode.lowerBound() == 1) {
    invariantGraph().replaceVarNode(quotient(), numerator());
    return true;
  }
  auto& nNode = invariantGraph().varNode(numerator());
  const auto& qNode = invariantGraph().varNode(quotient());
  assert((!nNode.isFixed() || !dNode.isFixed()) && qNode.isFixed() &&
         qNode.lowerBound() == 0);
  if (!nNode.isFixed() && !dNode.isFixed()) {
    if (nNode.lowerBound() >= 0 && dNode.lowerBound() >= 0) {
      invariantGraph().addInvariantNode(std::make_shared<IntLtNode>(
          invariantGraph(), numerator(), denominator()));
      return true;
    }
    if (nNode.upperBound() <= 0 && dNode.upperBound() <= 0) {
      invariantGraph().addInvariantNode(std::make_shared<IntLtNode>(
          invariantGraph(), denominator(), numerator()));
      return true;
    }
    if (nNode.lowerBound() >= 0) {
      assert(dNode.lowerBound() < 0);
      const auto dAbs =
          invariantGraph().retrieveIntVarNode(std::make_shared<SearchDomain>(
              1, std::max(dNode.upperBound(), -dNode.lowerBound())));
      invariantGraph().addInvariantNode(
          std::make_shared<IntAbsNode>(invariantGraph(), denominator(), dAbs));
      invariantGraph().addInvariantNode(
          std::make_shared<IntLtNode>(invariantGraph(), numerator(), dAbs));
      return true;
    }
    if (dNode.lowerBound() >= 0) {
      assert(nNode.lowerBound() < 0);
      const auto nAbs =
          invariantGraph().retrieveIntVarNode(std::make_shared<SearchDomain>(
              0, std::max(nNode.upperBound(), -nNode.lowerBound())));
      invariantGraph().addInvariantNode(
          std::make_shared<IntAbsNode>(invariantGraph(), numerator(), nAbs));
      invariantGraph().addInvariantNode(
          std::make_shared<IntLtNode>(invariantGraph(), nAbs, denominator()));
      return true;
    }
    assert(nNode.lowerBound() < 0);
    assert(dNode.lowerBound() < 0);
    const auto nAbs =
        invariantGraph().retrieveIntVarNode(std::make_shared<SearchDomain>(
            0, std::max(nNode.upperBound(), -nNode.lowerBound())));
    const auto dAbs =
        invariantGraph().retrieveIntVarNode(std::make_shared<SearchDomain>(
            1, std::max(dNode.upperBound(), -dNode.lowerBound())));
    invariantGraph().addInvariantNode(
        std::make_shared<IntAbsNode>(invariantGraph(), numerator(), nAbs));
    invariantGraph().addInvariantNode(
        std::make_shared<IntAbsNode>(invariantGraph(), denominator(), dAbs));
    invariantGraph().addInvariantNode(
        std::make_shared<IntLtNode>(invariantGraph(), nAbs, dAbs));
    return true;
  }
  if (!nNode.isFixed()) {
    assert(dNode.isFixed());
    const Int dVal = dNode.lowerBound();
    nNode.removeValuesBelow((dVal > 0 ? -dVal : dVal) + 1);
    nNode.removeValuesAbove((dVal < 0 ? -dVal : dVal) - 1);
    return true;
  }
  assert(!dNode.isFixed());
  assert(nNode.isFixed());
  const Int nVal = nNode.lowerBound();
  const Int lb = std::min(-nVal, nVal) + 1;
  const Int ub = std::max(-nVal, nVal) - 1;
  dNode.domain()->intersect(lb, ub - 1);
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

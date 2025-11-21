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

bool IntDivNode::updateNumerator() {
  auto& nNode = invariantGraph().varNode(numerator());
  const auto& dNode = invariantGraphConst().varNodeConst(denominator());
  const auto& qNode = invariantGraphConst().varNodeConst(quotient());

  if (qNode.isFixed() && qNode.lowerBound() == 0 && !nNode.isFixed() &&
      dNode.isFixed()) {
    assert(dNode.isFixed());
    const Int dVal = dNode.lowerBound();

    const Int pLb = nNode.lowerBound();
    const Int pUb = nNode.upperBound();
    nNode.removeValuesBelow((dVal > 0 ? -dVal : dVal) + 1);
    nNode.removeValuesAbove((dVal < 0 ? -dVal : dVal) - 1);
    setState(InvariantNodeState::SUBSUMED);
    return pLb != nNode.lowerBound() || pUb != nNode.upperBound();
  }

  // numerator
  const Int qLb = qNode.lowerBound();
  const Int qUb = qNode.upperBound();
  const Int dLb = dNode.lowerBound();
  const Int dUb = dNode.upperBound();

  const auto arr = std::array{std::pair{qLb, dLb}, std::pair{qLb, dUb},
                              std::pair{qUb, dLb}, std::pair{qUb, dUb}};

  Int newLb = std::numeric_limits<Int>::max();
  Int newUb = std::numeric_limits<Int>::min();
  for (auto& [q, d] : arr) {
    assert(d != 0);
    if (q == 0) {
      const Int n = std::abs(d) - 1;
      assert(q >= 0);
      newLb = std::min(newLb, -n);
      newUb = std::max(newUb, n);
      continue;
    }
    const bool nIsPos = (q >= 0) == (d >= 0);
    Int prod;
    if (__builtin_smull_overflow(q, d, &prod)) {
      if (nIsPos) {
        prod = std::numeric_limits<Int>::max();
      } else {
        prod = std::numeric_limits<Int>::min();
      }
    }
    assert(prod != 0);
    const Int remainder = std::abs(d) - 1;
    Int sum;
    if (__builtin_saddl_overflow(prod, prod > 0 ? remainder : -remainder,
                                 &sum)) {
      if (nIsPos) {
        sum = std::numeric_limits<Int>::max();
      } else {
        sum = std::numeric_limits<Int>::min();
      }
    }

    newLb = std::ranges::min(std::array{newLb, prod, sum});
    newUb = std::ranges::max(std::array{newUb, prod, sum});
  }

  const Int pLb = nNode.lowerBound();
  const Int pUb = nNode.upperBound();
  nNode.removeValuesBelow(newLb);
  nNode.removeValuesAbove(newUb);
  return pLb != nNode.lowerBound() || pUb != nNode.upperBound();
}

bool IntDivNode::updateDenominator() {
  // denominator
  const auto& nNode = invariantGraphConst().varNodeConst(numerator());
  auto& dNode = invariantGraph().varNode(denominator());
  const auto& qNode = invariantGraphConst().varNodeConst(quotient());

  if (qNode.isFixed() && qNode.lowerBound() == 0 && !dNode.isFixed() &&
      nNode.isFixed()) {
    const Int nVal = nNode.lowerBound();
    if (nVal == 0) {
      return false;
    }
    const Int holeLb = std::min(-nVal, nVal);
    const Int holeUb = std::max(-nVal, nVal);
    const Int pLb = dNode.lowerBound();
    const Int pUb = dNode.upperBound();
    for (Int v = std::max(holeLb, pLb); v <= std::min(holeUb, pUb); ++v) {
      dNode.removeValue(v);
    }
    setState(InvariantNodeState::SUBSUMED);
    return pLb != dNode.lowerBound() || pUb != dNode.upperBound();
  }

  return false;

  const Int nLb = nNode.lowerBound();
  const Int nUb = nNode.upperBound();
  const Int qLb = qNode.lowerBound() != 0
                      ? qNode.lowerBound()
                      : qNode.constDomain()->at(qNode.constDomain()->at(1));
  const Int qUb =
      qNode.upperBound() != 0
          ? qNode.upperBound()
          : qNode.constDomain()->at(qNode.constDomain()->size() - 2);
  const auto arr = std::array{std::pair{nLb, qLb}, std::pair{nLb, qUb},
                              std::pair{nUb, qLb}, std::pair{nUb, qUb}};
  Int newLb = std::numeric_limits<Int>::max();
  Int newUb = std::numeric_limits<Int>::min();
  for (const auto& [n, q] : arr) {
    if (n % q == 0) {
      newLb =
          std::ranges::min(std::array{newLb, div_floor(n, q), div_ceil(n, q)});
      newUb =
          std::ranges::max(std::array{newUb, div_floor(n, q), div_ceil(n, q)});
    } else {
      const Int offset = std::abs(q) - 1;
      newLb = std::ranges::min(
          std::array{newLb, div_floor(n, q) - offset, div_ceil(n, q) - offset});
      newUb = std::ranges::max(
          std::array{newUb, div_floor(n, q) - offset, div_ceil(n, q) - offset});
    }
  }
  const Int pLb = dNode.lowerBound();
  const Int pUb = dNode.upperBound();
  dNode.removeValuesBelow(newLb);
  dNode.removeValuesAbove(newUb);
  return pLb != dNode.lowerBound() || pUb != dNode.upperBound();
}

bool IntDivNode::updateQuotient() {
  // quotient
  const auto& nNode = invariantGraphConst().varNodeConst(numerator());
  const auto& dNode = invariantGraph().varNode(denominator());
  auto& qNode = invariantGraph().varNode(quotient());

  const bool dOverlapsZero = dNode.lowerBound() < 0 && 0 < dNode.upperBound();

  qNode.removeValuesBelow(std::min(nNode.lowerBound(), -nNode.upperBound()));
  qNode.removeValuesAbove(std::max(-nNode.lowerBound(), nNode.upperBound()));

  assert(!dNode.inDomain(Int{0}));
  const auto arr = std::array{
      std::pair{nNode.lowerBound(), dNode.lowerBound()},
      std::pair{nNode.lowerBound(), dNode.upperBound()},
      std::pair{nNode.lowerBound(), dOverlapsZero ? -1 : dNode.lowerBound()},
      std::pair{nNode.lowerBound(), dOverlapsZero ? 1 : dNode.lowerBound()},
      std::pair{nNode.upperBound(), dNode.lowerBound()},
      std::pair{nNode.upperBound(), dNode.upperBound()},
      std::pair{nNode.upperBound(), dOverlapsZero ? -1 : dNode.lowerBound()},
      std::pair{nNode.upperBound(), dOverlapsZero ? 1 : dNode.lowerBound()}};
  Int newLb = std::numeric_limits<Int>::max();
  Int newUb = std::numeric_limits<Int>::min();
  for (const auto& [n, d] : arr) {
    assert(d != 0);
    const Int q = n / d;
    newLb = std::min(newLb, q);
    newUb = std::max(newUb, q);
  }
  const Int pLb = qNode.lowerBound();
  const Int pUb = qNode.upperBound();
  qNode.removeValuesBelow(newLb);
  qNode.removeValuesAbove(newUb);
  return pLb != qNode.lowerBound() || pUb != qNode.upperBound();
}

void IntDivNode::updateState() {
  auto& dNode = invariantGraph().varNode(denominator());
  auto& nNode = invariantGraph().varNode(numerator());
  const auto& qNode = invariantGraphConst().varNodeConst(quotient());
  dNode.removeValue(Int{0});

  auto onStack = std::array{true, true, true};
  std::vector<size_t> stack{0, 1, 2};

  while (!stack.empty()) {
    const size_t index = stack.back();
    stack.pop_back();
    if (index == 0) {
      if (updateNumerator()) {
        if (state() != InvariantNodeState::ACTIVE) {
          return;
        }
        if (!onStack[1]) {
          stack.emplace_back(1);
        }
        if (!onStack[2]) {
          stack.emplace_back(2);
        }
      }
    } else if (index == 1) {
      if (updateDenominator()) {
        if (state() != InvariantNodeState::ACTIVE) {
          return;
        }
        if (!onStack[0]) {
          stack.emplace_back(0);
        }
        if (!onStack[2]) {
          stack.emplace_back(2);
        }
      }
    } else if (index == 2) {
      if (updateQuotient()) {
        if (state() != InvariantNodeState::ACTIVE) {
          return;
        }
        if (updateDenominator()) {
          if (!onStack[0]) {
            stack.emplace_back(0);
          }
          if (!onStack[1]) {
            stack.emplace_back(1);
          }
        }
      }
    }
    onStack[index] = false;
  }
  if (nNode.isFixed() && dNode.isFixed() && qNode.isFixed()) {
    const Int nVal = nNode.lowerBound();
    const Int dVal = dNode.lowerBound();
    const Int qVal = qNode.lowerBound();
    if (nVal / dVal != qVal) {
      throw InconsistencyException(
          "IntDivNode::update: " + std::to_string(nVal) + " / " +
          std::to_string(dVal) + " != " + std::to_string(qVal) + " (" +
          std::to_string(nVal / qVal) + ")");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (nNode.isFixed() && nNode.lowerBound() == 0) {
    assert(qNode.isFixed() && qNode.lowerBound() == 0);
    setState(InvariantNodeState::SUBSUMED);
    return;
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
  dNode.domain()->removeAllValuesExcept(lb, ub - 1);
  return true;
}

void IntDivNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  makeSolverVar(quotient(), solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntDivNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  assert(mapping.solverId(quotient()) != propagation::NULL_ID);
  assert(mapping.solverId(quotient()).isVar());

  solver.makeInvariant<propagation::IntDiv>(
      solver, mapping.solverId(quotient()), mapping.solverId(numerator()),
      mapping.solverId(denominator()));
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

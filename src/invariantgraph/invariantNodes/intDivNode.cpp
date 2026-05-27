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
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

IntDivNode::IntDivNode(InvariantGraph& graph, const VarNodeId numerator,
                       const VarNodeId denominator, const VarNodeId quotient)
    : InvariantNode(graph, {quotient}, {numerator, denominator}) {}

void IntDivNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(varNodeConst(quotient()).isIntVar());
  assert(varNodeConst(numerator()).isIntVar());
  assert(varNodeConst(denominator()).isIntVar());
}
void IntDivNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_div(varNodeConst(numerator()).constraintVarId(),
                             varNodeConst(denominator()).constraintVarId(),
                             varNodeConst(quotient()).constraintVarId());
}

void IntDivNode::updateState() {
  if (varNodeConst(quotient()).isFixed()) {
    auto& nNode = varNode(numerator());
    auto& dNode = varNode(denominator());
    const bool overZero = nNode.lowerBound() < 0 && 0 < nNode.upperBound() &&
                          dNode.lowerBound() < 0 && 0 < dNode.upperBound();
    if (!nNode.isFixed()) {
      nNode.tightenDomainType(overZero ? DomainType::DOM_DOMAIN
                                       : DomainType::DOM_RANGE);
    }
    if (!dNode.isFixed()) {
      dNode.tightenDomainType(overZero ? DomainType::DOM_DOMAIN
                                       : DomainType::DOM_RANGE);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntDivNode::canBeReplaced() const {
  const auto& nNode = varNodeConst(numerator());
  const auto& dNode = varNodeConst(denominator());
  const auto& qNode = varNodeConst(quotient());
  return state() == InvariantNodeState::ACTIVE &&
         ((dNode.isFixed() && dNode.lowerBound() == 1) ||
          ((!nNode.isFixed() || !dNode.isFixed()) && qNode.isFixed() &&
           qNode.lowerBound() == 0));
}

bool IntDivNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  auto& dNode = varNode(denominator());
  if (dNode.isFixed() && dNode.lowerBound() == 1) {
    invariantGraph().replaceVarNode(quotient(), numerator());
    return true;
  }
  auto& nNode = varNode(numerator());
  assert((!nNode.isFixed() || !dNode.isFixed()) &&
         varNode(quotient()).isFixed() &&
         varNode(quotient()).lowerBound() == 0);
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
    assert(dNode.lowerBound() > 0 ? -dNode.lowerBound() < nNode.lowerBound()
                                  : dNode.lowerBound() < nNode.lowerBound());
    assert(dNode.lowerBound() < 0 ? -dNode.lowerBound() > nNode.upperBound()
                                  : dNode.lowerBound() > nNode.upperBound());
    return true;
  }
  assert(!dNode.isFixed());
  assert(nNode.isFixed());
  assert(std::min(-nNode.lowerBound(), nNode.lowerBound()) + 1 <
         dNode.lowerBound());
  assert(dNode.upperBound() <
         std::max(-nNode.lowerBound(), nNode.lowerBound()) - 1);
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

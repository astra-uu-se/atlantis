#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/intModViewNode.hpp"
#include "atlantis/propagation/invariants/mod.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

IntModNode::IntModNode(InvariantGraph& graph, const VarNodeId numerator,
                       const VarNodeId denominator, const VarNodeId remainder)
    : InvariantNode(graph, {remainder}, {numerator, denominator}) {}

void IntModNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(varNodeConst(remainder()).isIntVar());
  assert(varNodeConst(numerator()).isIntVar());
  assert(varNodeConst(denominator()).isIntVar());
}
void IntModNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_mod(varNodeConst(numerator()).constraintVarId(),
                             varNodeConst(denominator()).constraintVarId(),
                             varNodeConst(remainder()).constraintVarId());
}

void IntModNode::updateState() {
  if (varNodeConst(remainder()).isFixed()) {
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

bool IntModNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         varNodeConst(denominator()).isFixed();
}

bool IntModNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(varNodeConst(denominator()).isFixed());
  invariantGraph().addInvariantNode(std::make_shared<IntModViewNode>(
      invariantGraph(), numerator(), remainder(),
      varNodeConst(denominator()).lowerBound()));
  return true;
}

void IntModNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntModNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());

  solver.makeInvariant<propagation::Mod>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(numerator()), mapping.solverId(denominator()));
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

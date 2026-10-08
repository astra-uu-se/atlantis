#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/pow.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

IntPowNode::IntPowNode(InvariantGraph& graph, VarNode& base, VarNode& exponent,
                       VarNode& power)
    : InvariantNode(graph, {power.ptr()}, {base.ptr(), exponent.ptr()}) {}

void IntPowNode::init() {
  InvariantNode::init();
  assert(
      outputVarNode(0).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void IntPowNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_pow(base().constraintVarId(),
                             exponent().constraintVarId(),
                             power().constraintVarId());
}

void IntPowNode::updateState() {
  if (base().isFixed() && exponent().isFixed()) {
    assert(power().isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntPowNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  makeSolverVar(outputVarNode(0), solver, mapping);
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void IntPowNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  assert(mapping.solverId(outputVarNode(0)) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNode(0)).isVar());

  solver.makeInvariant<propagation::Pow>(
      solver, mapping.solverId(outputVarNode(0)),
      mapping.solverId(base()), mapping.solverId(exponent()));
}

VarNode& IntPowNode::base() const {
  return *staticInputVarNodes().front();
}

VarNode& IntPowNode::exponent() const {
  return *staticInputVarNodes().back();
}

VarNode& IntPowNode::power() const {
  return outputVarNode(0);
}

std::string IntPowNode::dotLangIdentifier() const { return "int_pow"; }

}  // namespace atlantis::invariantgraph

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
    : InvariantNode(graph, {power}, {base, exponent}) {}

void IntPowNode::init() {
  InvariantNode::init();
  assert(
      invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
}

void IntPowNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_pow(varNodeConst(base()).constraintVarId(),
                             varNodeConst(exponent()).constraintVarId(),
                             varNodeConst(power()).constraintVarId());
}

void IntPowNode::updateState() {
  if (varNodeConst(base()).isFixed() && varNodeConst(exponent()).isFixed()) {
    assert(varNodeConst(power()).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntPowNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  makeSolverVar(outputVarNodes().front(), solver, mapping);
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void IntPowNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  assert(mapping.solverId(outputVarNodes().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodes().front()).isVar());

  solver.makeInvariant<propagation::Pow>(
      solver, mapping.solverId(outputVarNodes().front()),
      mapping.solverId(base()), mapping.solverId(exponent()));
}

const std::shared_ptr<VarNode>& IntPowNode::base() const {
  return staticInputVarNodes().front();
}

const std::shared_ptr<VarNode>& IntPowNode::exponent() const {
  return staticInputVarNodes().back();
}

const std::shared_ptr<VarNode>& IntPowNode::power() const {
  return outputVarNodes().front();
}

std::string IntPowNode::dotLangIdentifier() const { return "int_pow"; }

}  // namespace atlantis::invariantgraph

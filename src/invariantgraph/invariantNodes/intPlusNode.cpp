#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/plus.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

IntPlusNode::IntPlusNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                         VarNode& output)
    : InvariantNode(graph, {output}, {a, b}) {}

void IntPlusNode::init() {
  InvariantNode::init();
  assert(
      invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
}

void IntPlusNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_plus(staticInputVarNodeConst(0).constraintVarId(),
                              staticInputVarNodeConst(1).constraintVarId(),
                              outputVarNodeConst(0).constraintVarId());
}

void IntPlusNode::updateState() {
  std::vector<std::shared_ptr<VarNode>> varsToRemove;
  varsToRemove.reserve(staticInputVarNodes().size());

  for (const auto& input : staticInputVarNodes()) {
    if (input.isFixed()) {
      varsToRemove.emplace_back(input);
      _offset += input.lowerBound();
    }
  }

  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(input);
  }

  if (staticInputVarNodes().empty()) {
    assert(outputVarNodeConst(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntPlusNode::constrainsOutput(VarNode&) const {
  const Int lb = staticInputVarNodeConst(0).lowerBound() +
                 staticInputVarNodeConst(1).lowerBound();
  const Int ub = staticInputVarNodeConst(0).upperBound() +
                 staticInputVarNodeConst(1).upperBound();
  return !outputVarNodeConst(0).constDomain()->contains(lb, ub);
}

bool IntPlusNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodes().size() == 1;
}

bool IntPlusNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(staticInputVarNodes().size() == 1);
  if (_offset == 0) {
    invariantGraph().replaceVarNode(outputVarNodes().front(),
                                    staticInputVarNodes().front());
    return true;
  }
  invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(
      invariantGraph(), staticInputVarNodes().front(), outputVarNodes().front(),
      1, _offset));
  return true;
}

void IntPlusNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  assert(staticInputVarNodes().size() == 2);
  makeSolverVar(outputVarNodes().front(), solver, mapping);
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void IntPlusNode::registerNode(propagation::SolverBase& solver,
                               SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  assert(_offset == 0);
  assert(mapping.solverId(outputVarNodes().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodes().front()).isVar());

  solver.makeInvariant<propagation::Plus>(
      solver, mapping.solverId(outputVarNodes().front()),
      mapping.solverId(staticInputVarNodes().front()),
      mapping.solverId(staticInputVarNodes().back()));
}

std::string IntPlusNode::dotLangIdentifier() const { return "int_plus"; }

}  // namespace atlantis::invariantgraph

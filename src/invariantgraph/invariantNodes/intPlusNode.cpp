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
    : InvariantNode(graph, {output.ptr()}, {a.ptr(), b.ptr()}) {}

void IntPlusNode::init() {
  InvariantNode::init();
  assert(
      outputVarNode(0).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void IntPlusNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_plus(staticInputVarNode(0).constraintVarId(),
                              staticInputVarNode(1).constraintVarId(),
                              outputVarNode(0).constraintVarId());
}

void IntPlusNode::updateState() {
  std::vector<std::shared_ptr<VarNode>> varsToRemove;
  varsToRemove.reserve(staticInputVarNodes().size());

  for (const auto& vNode : staticInputVarNodes()) {
    if (vNode->isFixed()) {
      varsToRemove.emplace_back(vNode);
      _offset += vNode->lowerBound();
    }
  }

  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(*input);
  }

  if (staticInputVarNodes().empty()) {
    assert(outputVarNode(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntPlusNode::constrainsOutput(const VarNode&) const {
  const Int lb = staticInputVarNode(0).lowerBound() +
                 staticInputVarNode(1).lowerBound();
  const Int ub = staticInputVarNode(0).upperBound() +
                 staticInputVarNode(1).upperBound();
  return !outputVarNode(0).constDomain()->contains(lb, ub);
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
    invariantGraph().replaceVarNode(outputVarNode(0),
                                    staticInputVarNode(0));
    return true;
  }
  invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(
      invariantGraph(), staticInputVarNode(0), outputVarNode(0),
      1, _offset));
  return true;
}

void IntPlusNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  assert(staticInputVarNodes().size() == 2);
  makeSolverVar(outputVarNode(0), solver, mapping);
  assert(std::ranges::all_of(outputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return mapping.solverId(vNode) !=
                                      propagation::NULL_ID;
                             }));
}

void IntPlusNode::registerNode(propagation::SolverBase& solver,
                               SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  assert(_offset == 0);
  assert(mapping.solverId(outputVarNode(0)) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNode(0)).isVar());

  solver.makeInvariant<propagation::Plus>(
      solver, mapping.solverId(outputVarNode(0)),
      mapping.solverId(staticInputVarNode(0)),
      mapping.solverId(staticInputVarNodes().back()));
}

std::string IntPlusNode::dotLangIdentifier() const { return "int_plus"; }

}  // namespace atlantis::invariantgraph

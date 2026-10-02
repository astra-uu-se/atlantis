#include "atlantis/invariantgraph/views/intAbsNode.hpp"

#include <algorithm>
#include <numeric>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intAbsView.hpp"
#include "atlantis/utils/domains.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

IntAbsNode::IntAbsNode(InvariantGraph& graph, VarNode& staticInput,
                       VarNode& output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void IntAbsNode::init() {
  InvariantNode::init();
  assert(
      invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(
      invariantGraph().varNodeConst(staticInputVarNodes().front()).isIntVar());
}

void IntAbsNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_abs(staticInputVarNodeConst(0).constraintVarId(),
                             outputVarNodeConst(0).constraintVarId());
}

void IntAbsNode::updateState() {
  if (varNodeConst(input()).isFixed()) {
    assert(varNodeConst(input()).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAbsNode::constrainsOutput(VarNode&) const {
  const Int lb = staticInputVarNodeConst(0).lowerBound();
  const Int ub = staticInputVarNodeConst(0).upperBound();
  if (staticInputVarNodeConst(0).constDomain()->isInterval()) {
    if (lb >= 0) {
      return !outputVarNodeConst(0).constDomain()->contains(lb, ub);
    }
    if (ub <= 0) {
      return !outputVarNodeConst(0).constDomain()->contains(
          overflow::saturatingAbs(ub), overflow::saturatingAbs(lb));
    }
    return !outputVarNodeConst(0).constDomain()->contains(
        0, std::max(overflow::saturatingAbs(lb), ub));
  }
  if (lb >= 0) {
    return !outputVarNodeConst(0).constDomain()->contains(
        *staticInputVarNodeConst(0).constDomain());
  }
  std::vector<Int> vals(staticInputVarNodeConst(0).constDomain()->size());
  size_t i = 0;
  for (auto iter = staticInputVarNodeConst(0).constDomain()->begin();
       iter != staticInputVarNodeConst(0).constDomain()->end(); ++iter) {
    vals[i++] = overflow::saturatingAbs(*iter);
  }
  const SortedUniqueVector sortedVals(std::move(vals));
  return !outputVarNodeConst(0).constDomain()->contains(sortedVals);
}

bool IntAbsNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeConst(0).lowerBound() >= 0;
}

bool IntAbsNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().replaceVarNode(outputVarNodes().front(),
                                  staticInputVarNodes().front());
  return true;
}

void IntAbsNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodes().front()) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNodes().front(),
                        solver.makeIntView<propagation::IntAbsView>(
                            solver, mapping.solverId(input())));
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void IntAbsNode::registerNode(propagation::SolverBase&, SolverMapping&) const {}

std::string IntAbsNode::dotLangIdentifier() const { return "abs"; }

}  // namespace atlantis::invariantgraph

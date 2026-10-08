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
    : InvariantNode(graph, {output.ptr()}, {staticInput.ptr()}) {}

void IntAbsNode::init() {
  InvariantNode::init();
  assert(
      outputVarNode(0).isIntVar());
  assert(
      staticInputVarNode(0).isIntVar());
}

void IntAbsNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_abs(staticInputVarNode(0).constraintVarId(),
                             outputVarNode(0).constraintVarId());
}

void IntAbsNode::updateState() {
  if (input().isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAbsNode::constrainsOutput(const VarNode&) const {
  const Int lb = staticInputVarNode(0).lowerBound();
  const Int ub = staticInputVarNode(0).upperBound();
  if (staticInputVarNode(0).constDomain()->isInterval()) {
    if (lb >= 0) {
      return !outputVarNode(0).constDomain()->contains(lb, ub);
    }
    if (ub <= 0) {
      return !outputVarNode(0).constDomain()->contains(
          overflow::saturatingAbs(ub), overflow::saturatingAbs(lb));
    }
    return !outputVarNode(0).constDomain()->contains(
        0, std::max(overflow::saturatingAbs(lb), ub));
  }
  if (lb >= 0) {
    return !outputVarNode(0).constDomain()->contains(
        *staticInputVarNode(0).constDomain());
  }
  std::vector<Int> vals(staticInputVarNode(0).constDomain()->size());
  size_t i = 0;
  for (auto iter = staticInputVarNode(0).constDomain()->begin();
       iter != staticInputVarNode(0).constDomain()->end(); ++iter) {
    vals[i++] = overflow::saturatingAbs(*iter);
  }
  const SortedUniqueVector sortedVals(std::move(vals));
  return !outputVarNode(0).constDomain()->contains(sortedVals);
}

bool IntAbsNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNode(0).lowerBound() >= 0;
}

bool IntAbsNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().replaceVarNode(outputVarNode(0),
                                  staticInputVarNode(0));
  return true;
}

void IntAbsNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNode(0)) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNode(0),
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

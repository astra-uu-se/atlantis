#include "atlantis/invariantgraph/views/intAbsNode.hpp"

#include <algorithm>
#include <numeric>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intAbsView.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

IntAbsNode::IntAbsNode(InvariantGraph& graph, VarNodeId staticInput,
                       VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void IntAbsNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
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

bool IntAbsNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeConst(0).lowerBound() >= 0;
}

bool IntAbsNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  return true;
}

void IntAbsNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNodeIds().front(),
                        solver.makeIntView<propagation::IntAbsView>(
                            solver, mapping.solverId(input())));
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntAbsNode::registerNode(propagation::SolverBase&, SolverMapping&) const {}

std::string IntAbsNode::dotLangIdentifier() const { return "abs"; }

}  // namespace atlantis::invariantgraph

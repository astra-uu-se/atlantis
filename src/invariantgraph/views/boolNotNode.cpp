#include "atlantis/invariantgraph/views/boolNotNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

BoolNotNode::BoolNotNode(InvariantGraph& graph, VarNode& staticInput,
                         VarNode& output)
    : InvariantNode(graph, {output.ptr()}, {staticInput.ptr()}) {}

void BoolNotNode::init() {
  InvariantNode::init();
  assert(
      !outputVarNode(0).isIntVar());
  assert(
      !staticInputVarNode(0).isIntVar());
}
void BoolNotNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool_not(staticInputVarNode(0).constraintVarId(),
                              outputVarNode(0).constraintVarId(), true);
}

void BoolNotNode::updateState() {
  if (input().isFixed()) {
    assert(outputVarNode(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolNotNode::constrainsOutput(const VarNode&) const {
  if (staticInputVarNode(0).inDomain(bool{false}) &&
      !outputVarNode(0).inDomain(bool{true})) {
    return true;
  }
  if (staticInputVarNode(0).inDomain(bool{true}) &&
      !outputVarNode(0).inDomain(bool{false})) {
    return true;
  }
  return false;
}

bool BoolNotNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return staticInputVarNode(0).staticInputTo().size() ==
             1 &&
         staticInputVarNode(0).definingNodes().empty() &&
         !outputVarNode(0).staticInputTo().empty();
}

bool BoolNotNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(
      std::make_shared<BoolNotNode>(invariantGraph(), outputVarNode(0),
                                    staticInputVarNode(0)));
  return true;
}

void BoolNotNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNode(0)) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNode(0),
                        solver.makeIntView<propagation::Bool2IntView>(
                            solver, mapping.solverId(input())));
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void BoolNotNode::registerNode(propagation::SolverBase&, SolverMapping&) const {
}

std::string BoolNotNode::dotLangIdentifier() const { return "bool_not"; }

}  // namespace atlantis::invariantgraph

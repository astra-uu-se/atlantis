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
    : InvariantNode(graph, {output}, {staticInput}) {}

void BoolNotNode::init() {
  InvariantNode::init();
  assert(
      !invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(
      !invariantGraph().varNodeConst(staticInputVarNodes().front()).isIntVar());
}
void BoolNotNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool_not(staticInputVarNodeConst(0).constraintVarId(),
                              outputVarNodeConst(0).constraintVarId(), true);
}

void BoolNotNode::updateState() {
  if (varNodeConst(input()).isFixed()) {
    assert(outputVarNodeConst(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolNotNode::constrainsOutput(VarNode&) const {
  if (staticInputVarNodeConst(0).inDomain(bool{false}) &&
      !outputVarNodeConst(0).inDomain(bool{true})) {
    return true;
  }
  if (staticInputVarNodeConst(0).inDomain(bool{true}) &&
      !outputVarNodeConst(0).inDomain(bool{false})) {
    return true;
  }
  return false;
}

bool BoolNotNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return varNodeConst(staticInputVarNodes().front()).staticInputTo().size() ==
             1 &&
         varNodeConst(staticInputVarNodes().front()).definingNodes().empty() &&
         !varNodeConst(outputVarNodes().front()).staticInputTo().empty();
}

bool BoolNotNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(
      std::make_shared<BoolNotNode>(invariantGraph(), outputVarNodes().front(),
                                    staticInputVarNodes().front()));
  return true;
}

void BoolNotNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodes().front()) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNodes().front(),
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

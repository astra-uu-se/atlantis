#include "atlantis/invariantgraph/views/int2BoolNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/bool2IntNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/int2BoolView.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

Int2BoolNode::Int2BoolNode(InvariantGraph& graph, VarNode& staticInput,
                           VarNode& output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void Int2BoolNode::init() {
  InvariantNode::init();
  assert(
      !invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(
      invariantGraph().varNodeConst(staticInputVarNodes().front()).isIntVar());
}

void Int2BoolNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool2int(outputVarNodeConst(0).constraintVarId(),
                              staticInputVarNodeConst(0).constraintVarId());
}

void Int2BoolNode::updateState() {
  assert(outputVarNodeConst(0).isFixed() == varNodeConst(input()).isFixed());
  if (varNodeConst(input()).isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool Int2BoolNode::constrainsOutput(VarNode&) const {
  if (staticInputVarNodeConst(0).inDomain(Int{0}) &&
      !outputVarNodeConst(0).inDomain(bool{false})) {
    return true;
  }
  if (staticInputVarNodeConst(0).inDomain(Int{1}) &&
      !outputVarNodeConst(0).inDomain(bool{true})) {
    return true;
  }
  return false;
}

bool Int2BoolNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return varNodeConst(staticInputVarNodes().front()).staticInputTo().size() ==
             1 &&
         varNodeConst(staticInputVarNodes().front()).definingNodes().empty() &&
         !varNodeConst(outputVarNodes().front()).staticInputTo().empty();
}

bool Int2BoolNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(
      std::make_shared<Bool2IntNode>(invariantGraph(), outputVarNodes().front(),
                                     staticInputVarNodes().front()));
  return true;
}

void Int2BoolNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodes().front()) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNodes().front(),
                        solver.makeIntView<propagation::Int2BoolView>(
                            solver, mapping.solverId(input())));
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void Int2BoolNode::registerNode(propagation::SolverBase&,
                                SolverMapping&) const {}

std::string Int2BoolNode::dotLangIdentifier() const { return "int2bool"; }

}  // namespace atlantis::invariantgraph

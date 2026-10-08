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
    : InvariantNode(graph, {output.ptr()}, {staticInput.ptr()}) {}

void Int2BoolNode::init() {
  InvariantNode::init();
  assert(
      !outputVarNode(0).isIntVar());
  assert(
      staticInputVarNode(0).isIntVar());
}

void Int2BoolNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool2int(outputVarNode(0).constraintVarId(),
                              staticInputVarNode(0).constraintVarId());
}

void Int2BoolNode::updateState() {
  assert(outputVarNode(0).isFixed() == input().isFixed());
  if (input().isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool Int2BoolNode::constrainsOutput(const VarNode&) const {
  if (staticInputVarNode(0).inDomain(Int{0}) &&
      !outputVarNode(0).inDomain(bool{false})) {
    return true;
  }
  if (staticInputVarNode(0).inDomain(Int{1}) &&
      !outputVarNode(0).inDomain(bool{true})) {
    return true;
  }
  return false;
}

bool Int2BoolNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return staticInputVarNode(0).staticInputTo().size() ==
             1 &&
         staticInputVarNode(0).definingNodes().empty() &&
         !outputVarNode(0).staticInputTo().empty();
}

bool Int2BoolNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(
      std::make_shared<Bool2IntNode>(invariantGraph(), outputVarNode(0),
                                     staticInputVarNode(0)));
  return true;
}

void Int2BoolNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNode(0)) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNode(0),
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

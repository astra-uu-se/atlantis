#include "atlantis/invariantgraph/views/bool2IntNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/int2BoolNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/bool2IntView.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

Bool2IntNode::Bool2IntNode(InvariantGraph& graph, VarNode& staticInput,
                           VarNode& output)
    : InvariantNode(graph, {output.ptr()}, {staticInput.ptr()}) {}

void Bool2IntNode::init() {
  InvariantNode::init();
  assert(!staticInputVarNode(0).isIntVar());
  assert(outputVarNode(0).isIntVar());
}

void Bool2IntNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool2int(staticInputVarNode(0).constraintVarId(),
                              outputVarNode(0).constraintVarId());
}

void Bool2IntNode::updateState() {
  assert(outputVarNode(0).isFixed() == input().isFixed());
  if (input().isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool Bool2IntNode::constrainsOutput(const VarNode&) const {
  if (staticInputVarNode(0).inDomain(bool{false}) &&
      !outputVarNode(0).inDomain(Int{0})) {
    return true;
  }
  if (staticInputVarNode(0).inDomain(bool{true}) &&
      !outputVarNode(0).inDomain(Int{1})) {
    return true;
  }
  return false;
}

bool Bool2IntNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return staticInputVarNode(0).staticInputTo().size() ==
             1 &&
         staticInputVarNode(0).definingNodes().empty() &&
         !outputVarNode(0).staticInputTo().empty();
}

bool Bool2IntNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(
      std::make_shared<Int2BoolNode>(invariantGraph(), outputVarNode(0),
                                     staticInputVarNode(0)));
  return true;
}

void Bool2IntNode::registerOutputVars(propagation::SolverBase& solver,
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

void Bool2IntNode::registerNode(propagation::SolverBase&,
                                SolverMapping&) const {}

std::string Bool2IntNode::dotLangIdentifier() const { return "bool2int"; }

}  // namespace atlantis::invariantgraph

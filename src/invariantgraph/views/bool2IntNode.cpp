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

Bool2IntNode::Bool2IntNode(InvariantGraph& graph, const VarNodeId staticInput,
                           const VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void Bool2IntNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(!invariantGraph()
              .varNodeConst(staticInputVarNodeIds().front())
              .isIntVar());
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
}

void Bool2IntNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool2int(staticInputVarNodeConst(0).constraintVarId(),
                              outputVarNodeConst(0).constraintVarId());
}

void Bool2IntNode::updateState() {
  assert(outputVarNodeConst(0).isFixed() == varNodeConst(input()).isFixed());
  if (varNodeConst(input()).isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool Bool2IntNode::constrainsOutput(VarNodeId) const {
  if (staticInputVarNodeConst(0).inDomain(bool{false}) && !outputVarNodeConst(0).inDomain(Int{0})) {
    return true;
  }
  if (staticInputVarNodeConst(0).inDomain(bool{true}) && !outputVarNodeConst(0).inDomain(Int{1})) {
    return true;
  }
  return false;
}

bool Bool2IntNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return varNodeConst(staticInputVarNodeIds().front()).staticInputTo().size() == 1 && varNodeConst(staticInputVarNodeIds().front()).definingNodes().empty() && !varNodeConst(outputVarNodeIds().front()).staticInputTo().empty();
}

bool Bool2IntNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(std::make_shared<Int2BoolNode>(invariantGraph(), outputVarNodeIds().front(), staticInputVarNodeIds().front()));
  return true;
}

void Bool2IntNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNodeIds().front(),
                        solver.makeIntView<propagation::Bool2IntView>(
                            solver, mapping.solverId(input())));
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void Bool2IntNode::registerNode(propagation::SolverBase&,
                                SolverMapping&) const {}

std::string Bool2IntNode::dotLangIdentifier() const { return "bool2int"; }

}  // namespace atlantis::invariantgraph

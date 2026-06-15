#include "atlantis/invariantgraph/views/bool2IntNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
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

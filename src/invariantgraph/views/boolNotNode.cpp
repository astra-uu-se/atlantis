#include "atlantis/invariantgraph/views/boolNotNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

BoolNotNode::BoolNotNode(InvariantGraph& graph, VarNodeId staticInput,
                         VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void BoolNotNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(!invariantGraphConst()
              .varNodeConst(outputVarNodeIds().front())
              .isIntVar());
  assert(!invariantGraph()
              .varNodeConst(staticInputVarNodeIds().front())
              .isIntVar());
}
void BoolNotNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().bool_not(staticInputVarNodeConst(0).constraintVarId(), outputVarNodeConst(0).constraintVarId(), true);
}

void BoolNotNode::updateState() {
  if (invariantGraphConst().varNodeConst(input()).isFixed()) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .fixToValue(
            !invariantGraphConst().varNodeConst(input()).inDomain(bool{true}));
    setState(InvariantNodeState::SUBSUMED);
  } else if (invariantGraph()
                 .varNodeConst(outputVarNodeIds().front())
                 .isFixed()) {
    invariantGraph().varNode(input()).fixToValue(
        !invariantGraph()
             .varNodeConst(outputVarNodeIds().front())
             .inDomain(bool{true}));
    setState(InvariantNodeState::SUBSUMED);
  }
}

void BoolNotNode::registerOutputVars(propagation::SolverBase& solver,
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

void BoolNotNode::registerNode(propagation::SolverBase&, SolverMapping&) const {
}

std::string BoolNotNode::dotLangIdentifier() const { return "bool_not"; }

}  // namespace atlantis::invariantgraph

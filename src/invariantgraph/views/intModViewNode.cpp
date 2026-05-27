#include "atlantis/invariantgraph/views/intModViewNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::invariantgraph {

IntModViewNode::IntModViewNode(InvariantGraph& graph, VarNodeId staticInput,
                               VarNodeId output, Int denominator)
    : InvariantNode(graph, {output}, {staticInput}),
      _denominator(std::abs(denominator)) {}

void IntModViewNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}
void IntModViewNode::postConstraint() {
  InvariantNode::postConstraint();
  const auto den = invariantGraph().retrieveIntVarNode(_denominator);
  constraintSolver().int_mod(staticInputVarNodeConst(0).constraintVarId(), varNodeConst(den).constraintVarId(), outputVarNodeConst(0).constraintVarId());
}

void IntModViewNode::updateState() {
  if (staticInputVarNodeConst(0).isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntModViewNode::registerOutputVars(propagation::SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNodeIds().front(),
                        solver.makeIntView<propagation::ModView>(
                            solver, mapping.solverId(input()), _denominator));
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntModViewNode::registerNode(propagation::SolverBase&,
                                  SolverMapping&) const {}

std::string IntModViewNode::dotLangIdentifier() const {
  return "% " + std::to_string(_denominator);
}

}  // namespace atlantis::invariantgraph

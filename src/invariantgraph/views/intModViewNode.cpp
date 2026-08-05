#include "atlantis/invariantgraph/views/intModViewNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::invariantgraph {

IntModViewNode::IntModViewNode(InvariantGraph& graph,
                               const VarNodeId staticInput,
                               const VarNodeId output, const Int denominator)
    : InvariantNode(graph, {output}, {staticInput}),
      _denominator(std::abs(denominator)) {}

void IntModViewNode::init(const InvariantNodeId id) {
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
  constraintSolver().int_mod(staticInputVarNodeConst(0).constraintVarId(),
                             varNodeConst(den).constraintVarId(),
                             outputVarNodeConst(0).constraintVarId());
}

void IntModViewNode::updateState() {
  if (staticInputVarNodeConst(0).isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntModViewNode::constrainsOutput(VarNodeId) const {
  return !outputVarNodeConst(0).constDomain()->contains(0, _denominator - 1);
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

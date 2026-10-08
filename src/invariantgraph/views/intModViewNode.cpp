#include "atlantis/invariantgraph/views/intModViewNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::invariantgraph {

IntModViewNode::IntModViewNode(InvariantGraph& graph, VarNode& staticInput,
                               VarNode& output, const Int denominator)
    : InvariantNode(graph, {output.ptr()}, {staticInput.ptr()}),
      _denominator(std::abs(denominator)) {}

void IntModViewNode::init() {
  InvariantNode::init();
  assert(
      outputVarNode(0).isIntVar());
  assert(
      staticInputVarNode(0).isIntVar());
}

void IntModViewNode::postConstraint() {
  InvariantNode::postConstraint();
  const auto den = invariantGraph().retrieveIntVarNode(_denominator);
  constraintSolver().int_mod(staticInputVarNode(0).constraintVarId(),
                             den.constraintVarId(),
                             outputVarNode(0).constraintVarId());
}

void IntModViewNode::updateState() {
  if (staticInputVarNode(0).isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntModViewNode::constrainsOutput(const VarNode&) const {
  return !outputVarNode(0).constDomain()->contains(0, _denominator - 1);
}

void IntModViewNode::registerOutputVars(propagation::SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNode(0)) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNode(0),
                        solver.makeIntView<propagation::ModView>(
                            solver, mapping.solverId(input()), _denominator));
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void IntModViewNode::registerNode(propagation::SolverBase&,
                                  SolverMapping&) const {}

std::string IntModViewNode::dotLangIdentifier() const {
  return "% " + std::to_string(_denominator);
}

}  // namespace atlantis::invariantgraph

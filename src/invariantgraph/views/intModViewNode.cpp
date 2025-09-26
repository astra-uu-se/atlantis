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

void IntModViewNode::updateState() {
  auto& numerator = invariantGraph().varNode(staticInputVarNodeIds().front());
  auto& remainder = invariantGraph().varNode(outputVarNodeIds().front());

  if (numerator.lowerBound() >= 0) {
    remainder.removeValuesBelow(0);
  }
  if (numerator.upperBound() <= 0) {
    remainder.removeValuesAbove(0);
  }
  if (remainder.lowerBound() > 0) {
    numerator.removeValuesBelow(0);
  }
  if (remainder.upperBound() < 0) {
    numerator.removeValuesAbove(0);
  }

  if (numerator.isFixed()) {
    remainder.fixToValue(numerator.lowerBound() % _denominator);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  const Int lb = std::min(-_denominator + 1, Int{0});
  const Int ub = std::max(_denominator - 1, Int{0});
  remainder.removeValuesBelow(lb);
  remainder.removeValuesAbove(ub);
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

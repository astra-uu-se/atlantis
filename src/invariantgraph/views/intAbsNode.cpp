#include "atlantis/invariantgraph/views/intAbsNode.hpp"

#include <algorithm>
#include <numeric>

#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intAbsView.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

IntAbsNode::IntAbsNode(InvariantGraph& graph, VarNodeId staticInput,
                       VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void IntAbsNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}
void IntAbsNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_abs(staticInputVarNodeConst(0).constraintVarId(),
                             outputVarNodeConst(0).constraintVarId());
}

void IntAbsNode::updateState() {
  auto& iNode = invariantGraph().varNode(staticInputVarNodeIds().front());
  if (iNode.lowerBound() >= 0) {
    // will be replaced by equality constraint
    return;
  }

  auto& oNode = invariantGraph().varNode(outputVarNodeIds().front());
  oNode.removeValuesBelow(0);

  if (iNode.lowerBound() < 0 && iNode.upperBound() <= 0) {
    iNode.removeValuesBelow(-oNode.upperBound());
    iNode.removeValuesAbove(-oNode.lowerBound());

    oNode.removeValuesBelow(-iNode.upperBound());
    oNode.removeValuesAbove(-iNode.lowerBound());

    if (iNode.domain()->size() < oNode.domain()->size()) {
      std::vector<Int> oppDom(iNode.domain()->size());
      for (Int i = static_cast<Int>(oppDom.size()) - 1; i >= 0; i--) {
        oppDom[i] = -iNode.domain()->at(static_cast<Int>(oppDom.size() - i));
      }
      oNode.removeAllValuesExcept(SortedUniqueVector(std::move(oppDom)));
    } else if (oNode.domain()->size() < iNode.domain()->size()) {
      std::vector<Int> oppDom(oNode.domain()->size());
      for (Int i = static_cast<Int>(oppDom.size()) - 1; i >= 0; i--) {
        oppDom[i] = -oNode.domain()->at(static_cast<Int>(oppDom.size() - i));
      }
      iNode.removeAllValuesExcept(SortedUniqueVector(std::move(oppDom)));
    }
  } else {
    iNode.removeValuesBelow(std::min(oNode.lowerBound(), -oNode.upperBound()));
    iNode.removeValuesAbove(std::max(oNode.upperBound(), -oNode.lowerBound()));
    oNode.removeValuesAbove(std::max(-iNode.lowerBound(), iNode.upperBound()));
  }
  if (invariantGraph()
          .varNodeConst(staticInputVarNodeIds().front())
          .isFixed()) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .fixToValue(std::abs(invariantGraph()
                                 .varNodeConst(staticInputVarNodeIds().front())
                                 .lowerBound()));
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAbsNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         invariantGraphConst()
                 .varNodeConst(staticInputVarNodeIds().front())
                 .lowerBound() >= 0;
}

bool IntAbsNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  return true;
}

void IntAbsNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (mapping.solverId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    mapping.setSolverId(outputVarNodeIds().front(),
                        solver.makeIntView<propagation::IntAbsView>(
                            solver, mapping.solverId(input())));
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntAbsNode::registerNode(propagation::SolverBase&, SolverMapping&) const {}

std::string IntAbsNode::dotLangIdentifier() const { return "abs"; }

}  // namespace atlantis::invariantgraph

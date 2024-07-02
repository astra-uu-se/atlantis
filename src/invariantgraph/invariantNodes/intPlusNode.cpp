#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

#include <cmath>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/plus.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

IntPlusNode::IntPlusNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

void IntPlusNode::updateState(InvariantGraph& graph) {
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).isFixed()) {
      varsToRemove.emplace_back(input);
      _offset += graph.varNodeConst(input).lowerBound();
    }
  }

  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }

  if (staticInputVarNodeIds().empty()) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntPlusNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1 && _offset == 0;
}

bool IntPlusNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    graph.replaceVarNode(outputVarNodeIds().front(),
                         staticInputVarNodeIds().front());
  }
  return true;
}

void IntPlusNode::registerOutputVars(InvariantGraph& invariantGraph,
                                     propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  } else if (_offset != 0) {
    if (staticInputVarNodeIds().size() == 1) {
      invariantGraph.varNode(outputVarNodeIds().front())
          .setVarId(solver.makeIntView<propagation::IntOffsetView>(
              solver, invariantGraph.varId(staticInputVarNodeIds().front()),
              _offset));
    } else {
      _intermediate = solver.makeIntVar(0, 0, 0);
      invariantGraph.varNode(outputVarNodeIds().front())
          .setVarId(solver.makeIntView<propagation::IntOffsetView>(
              solver, _intermediate, _offset));
    }
  } else {
    makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
  }
}

void IntPlusNode::registerNode(InvariantGraph& invariantGraph,
                               propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  solver.makeInvariant<propagation::Plus>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(staticInputVarNodeIds().front()),
      invariantGraph.varId(staticInputVarNodeIds().back()));
}

}  // namespace atlantis::invariantgraph

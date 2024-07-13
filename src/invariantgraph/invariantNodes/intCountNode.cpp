#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntCountNode::IntCountNode(std::vector<VarNodeId>&& vars, Int needle,
                           VarNodeId count)
    : InvariantNode(std::vector<VarNodeId>{count}, std::move(vars)),
      _needle(needle) {}

const std::vector<VarNodeId>& IntCountNode::haystack() const {
  return staticInputVarNodeIds();
}

void IntCountNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void IntCountNode::updateState(InvariantGraph& graph) {
  std::vector<VarNodeId> inputsToRemove;
  inputsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& input : staticInputVarNodeIds()) {
    const auto& inputNode = graph.varNodeConst(input);
    if (inputNode.isFixed() || !inputNode.inDomain(needle())) {
      _offset += inputNode.lowerBound() == needle() ? 1 : 0;
      inputsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : inputsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  auto& outputNode = graph.varNode(outputVarNodeIds().front());
  const Int lb = _offset;
  const Int ub = _offset + static_cast<Int>(staticInputVarNodeIds().size());
  outputNode.removeValuesBelow(lb);
  outputNode.removeValuesAbove(ub);
  if (outputNode.isFixed()) {
    for (const auto& input : staticInputVarNodeIds()) {
      graph.varNode(input).removeValue(_needle);
    }
  }
  if (staticInputVarNodeIds().empty() || outputNode.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

Int IntCountNode::needle() const { return _needle; }

void IntCountNode::registerOutputVars(InvariantGraph& graph,
                                      propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() == 1) {
    graph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IfThenElseConst>(
            solver, graph.varId(staticInputVarNodeIds().front()), _offset + 1,
            _offset, needle()));
  } else if (!staticInputVarNodeIds().empty()) {
    if (_offset == 0) {
      makeSolverVar(solver, graph.varNode(outputVarNodeIds().front()));
    } else if (_intermediate != propagation::NULL_ID) {
      _intermediate = solver.makeIntVar(0, 0, 0);
      graph.varNode(outputVarNodeIds().front())
          .setVarId(solver.makeIntView<propagation::IntOffsetView>(
              solver, _intermediate, _offset));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntCountNode::registerNode(InvariantGraph& graph,
                                propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());

  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const VarNodeId node) { return graph.varId(node); });

  solver.makeInvariant<propagation::CountConst>(
      solver,
      _intermediate == propagation::NULL_ID
          ? graph.varId(outputVarNodeIds().front())
          : _intermediate,
      needle(), std::move(solverVars));
}

}  // namespace atlantis::invariantgraph

#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

IntCountNode::IntCountNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           Int needle, VarNodeId count, Int offset)
    : InvariantNode(graph, std::vector<VarNodeId>{count}, std::move(vars)),
      _needle(needle),
      _offset(offset) {}

const std::vector<VarNodeId>& IntCountNode::haystack() const {
  return staticInputVarNodeIds();
}

void IntCountNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void IntCountNode::updateState() {
  std::vector<VarNodeId> inputsToRemove;
  inputsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& input : staticInputVarNodeIds()) {
    const auto& inputNode = invariantGraphConst().varNodeConst(input);
    if (inputNode.isFixed() || !inputNode.inDomain(needle())) {
      _offset += inputNode.lowerBound() == needle() ? 1 : 0;
      inputsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : inputsToRemove) {
    removeStaticInputVarNode(input);
  }
  auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());
  const Int ub = _offset + static_cast<Int>(staticInputVarNodeIds().size());
  outputNode.removeValuesBelow(_offset);
  outputNode.removeValuesAbove(ub);
  if (outputNode.isFixed()) {
    if (outputNode.lowerBound() == _offset) {
      for (const auto& input : staticInputVarNodeIds()) {
        invariantGraph().varNode(input).removeValue(_needle);
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    if (outputNode.lowerBound() == ub) {
      for (const auto& input : staticInputVarNodeIds()) {
        invariantGraph().varNode(input).fixToValue(_needle);
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

Int IntCountNode::needle() const { return _needle; }

void IntCountNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::IfThenElseConst>(
            solver, mapping.solverId(staticInputVarNodeIds().front()),
            _offset + 1, _offset, needle()));
  } else if (!staticInputVarNodeIds().empty()) {
    if (_offset == 0) {
      makeSolverVar(outputVarNodeIds().front(), solver, mapping);
    } else if (mapping.intermediateId(id()) == propagation::NULL_ID) {
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      mapping.setSolverId(outputVarNodeIds().front(),
                          solver.makeIntView<propagation::IntOffsetView>(
                              solver, mapping.intermediateId(id()), _offset));
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntCountNode::registerNode(propagation::SolverBase& solver,
                                SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()) == propagation::NULL_ID
             ? mapping.solverId(outputVarNodeIds().front()).isVar()
             : mapping.solverId(outputVarNodeIds().front()).isView());
  assert(mapping.intermediateId(id()) == propagation::NULL_ID ||
         mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());

  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(solverVars),
      [&](const VarNodeId node) { return mapping.solverId(node); });

  solver.makeInvariant<propagation::CountConst>(
      solver,
      mapping.intermediateId(id()) == propagation::NULL_ID
          ? mapping.solverId(outputVarNodeIds().front())
          : mapping.intermediateId(id()),
      needle(), std::move(solverVars));
}

std::string IntCountNode::dotLangIdentifier() const {
  return "int_count " + std::to_string(_needle);
}

}  // namespace atlantis::invariantgraph

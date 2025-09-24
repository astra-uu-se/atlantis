#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/times.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntTimesNode::IntTimesNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                           VarNodeId output)
    : InvariantNode(graph, {output}, {a, b}) {}

void IntTimesNode::init(InvariantNodeId id) {
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

void IntTimesNode::updateState() {
  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& varNodeId : staticInputVarNodeIds()) {
    if (invariantGraphConst().varNodeConst(varNodeId).isFixed()) {
      varNodeIdsToRemove.emplace_back(varNodeId);
      _scalar *= invariantGraphConst().varNodeConst(varNodeId).lowerBound();
    }
  }

  if (_scalar == 0) {
    invariantGraph().varNode(outputVarNodeIds().front()).fixToValue(Int{0});
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& varNodeId : varNodeIdsToRemove) {
    removeStaticInputVarNode(varNodeId);
  }

  Int lb = _scalar;
  Int ub = _scalar;
  for (const auto& inputId : staticInputVarNodeIds()) {
    const auto& inputNode = invariantGraphConst().varNodeConst(inputId);
    lb = std::min(lb * inputNode.lowerBound(), lb * inputNode.upperBound());
    ub = std::max(ub * inputNode.lowerBound(), ub * inputNode.upperBound());
  }

  auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());

  outputNode.removeValuesBelow(lb);
  outputNode.removeValuesAbove(ub);

  if (staticInputVarNodeIds().size() == 1 && _scalar != 0) {
    const Int v1 = outputNode.lowerBound() / _scalar;
    const Int v2 = outputNode.upperBound() / _scalar;
    auto& inputNode = invariantGraph().varNode(staticInputVarNodeIds().front());
    inputNode.removeValuesBelow(std::min(v1, v2));
    inputNode.removeValuesAbove(std::max(v1, v2));
  }

  if (outputNode.isFixed() && staticInputVarNodeIds().size() == 1) {
    const Int numerator = outputNode.lowerBound();
    if (numerator % _scalar != 0) {
      throw InconsistencyException(
          "IntTimesNode::updateState: fixed output must be divisible by scalar "
          "(" +
          std::to_string(numerator) + " % " + std::to_string(_scalar) + " = " +
          std::to_string(numerator % _scalar) + ").");
    }
    invariantGraph()
        .varNode(staticInputVarNodeIds().front())
        .fixToValue(numerator / _scalar);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntTimesNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1 && _scalar == 1;
}

bool IntTimesNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }

  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
  }

  return true;
}

void IntTimesNode::registerOutputVars(propagation::SolverBase& solver, SolverMapping& mapping) const {
  if (!staticInputVarNodeIds().empty()) {
    if (_scalar != 1) {
      if (staticInputVarNodeIds().size() == 1) {
        mapping.setSolverId(
            outputVarNodeIds().front(),
            solver.makeIntView<propagation::ScalarView>(
                solver,
                mapping.solverId(staticInputVarNodeIds().front()),
                _scalar));
      } else {
        mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
        mapping.setSolverId(
            outputVarNodeIds().front(),
            solver.makeIntView<propagation::ScalarView>(
                solver, mapping.intermediateId(id()), _scalar));
      }
    } else {
      makeSolverVar(outputVarNodeIds().front(), solver, mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) !=
               propagation::NULL_ID;
      }));
}

void IntTimesNode::registerNode(propagation::SolverBase& solver, SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  assert(mapping.solverId(outputVarNodeIds().front()).isVar());

  solver.makeInvariant<propagation::Times>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(staticInputVarNodeIds().front()),
      mapping.solverId(staticInputVarNodeIds().back()));
}

std::string IntTimesNode::dotLangIdentifier() const { return "int_times"; }

}  // namespace atlantis::invariantgraph

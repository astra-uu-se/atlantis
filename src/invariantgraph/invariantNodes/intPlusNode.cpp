#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

#include <cmath>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/plus.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

IntPlusNode::IntPlusNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                         VarNodeId output)
    : InvariantNode(graph, {output}, {a, b}) {}

void IntPlusNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void IntPlusNode::updateState() {
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (invariantGraphConst().varNodeConst(input).isFixed()) {
      varsToRemove.emplace_back(input);
      _offset += invariantGraphConst().varNodeConst(input).lowerBound();
    }
  }

  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(input);
  }

  if (staticInputVarNodeIds().empty()) {
    invariantGraph().varNode(outputVarNodeIds().front()).fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntPlusNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1 && _offset == 0;
}

bool IntPlusNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
  }
  return true;
}

void IntPlusNode::registerOutputVars() {
  if (!staticInputVarNodeIds().empty()) {
    if (_offset != 0) {
      if (staticInputVarNodeIds().size() == 1) {
        invariantGraph()
            .varNode(outputVarNodeIds().front())
            .setVarId(solver().makeIntView<propagation::IntOffsetView>(
                solver(),
                invariantGraph().varId(staticInputVarNodeIds().front()),
                _offset));
      } else {
        _intermediate = solver().makeIntVar(0, 0, 0);
        invariantGraph()
            .varNode(outputVarNodeIds().front())
            .setVarId(solver().makeIntView<propagation::IntOffsetView>(
                solver(), _intermediate, _offset));
      }
    } else {
      makeSolverVar(outputVarNodeIds().front());
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntPlusNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  solver().makeInvariant<propagation::Plus>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(staticInputVarNodeIds().front()),
      invariantGraph().varId(staticInputVarNodeIds().back()));
}

std::string IntPlusNode::dotLangIdentifier() const { return "int_plus"; }

}  // namespace atlantis::invariantgraph

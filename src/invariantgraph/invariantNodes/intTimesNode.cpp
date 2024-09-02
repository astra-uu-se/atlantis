#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/times.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntTimesNode::IntTimesNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                           VarNodeId output)
    : InvariantNode(graph, {output}, {a, b}) {}

void IntTimesNode::init(InvariantNodeId id) {
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

  // auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());

  // outputNode.removeValuesBelow(lb);
  // outputNode.removeValuesAbove(ub);

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

void IntTimesNode::registerOutputVars() {
  if (!staticInputVarNodeIds().empty()) {
    if (_scalar != 1) {
      if (staticInputVarNodeIds().size() == 1) {
        invariantGraph()
            .varNode(outputVarNodeIds().front())
            .setVarId(solver().makeIntView<propagation::ScalarView>(
                solver(),
                invariantGraph().varId(staticInputVarNodeIds().front()),
                _scalar));
      } else {
        _intermediate = solver().makeIntVar(0, 0, 0);
        invariantGraph()
            .varNode(outputVarNodeIds().front())
            .setVarId(solver().makeIntView<propagation::ScalarView>(
                solver(), _intermediate, _scalar));
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

void IntTimesNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  solver().makeInvariant<propagation::Times>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(staticInputVarNodeIds().front()),
      invariantGraph().varId(staticInputVarNodeIds().back()));
}

std::string IntTimesNode::dotLangIdentifier() const { return "int_times"; }

}  // namespace atlantis::invariantgraph

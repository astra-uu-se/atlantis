#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntCountNode::IntCountNode(IInvariantGraph& graph,
                           std::vector<VarNodeId>&& vars, Int needle,
                           VarNodeId count)
    : InvariantNode(graph, std::vector<VarNodeId>{count}, std::move(vars)),
      _needle(needle) {}

const std::vector<VarNodeId>& IntCountNode::haystack() const {
  return staticInputVarNodeIds();
}

void IntCountNode::init(InvariantNodeId id) {
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
  // const Int lb = _offset;
  // const Int ub = _offset + static_cast<Int>(staticInputVarNodeIds().size());
  // outputNode.removeValuesBelow(lb);
  // outputNode.removeValuesAbove(ub);
  // if (outputNode.isFixed()) {
  //  for (const auto& input : staticInputVarNodeIds()) {
  //    invariantGraph().varNode(input).removeValue(_needle);
  //  }
  //}
  if (staticInputVarNodeIds().empty() || outputNode.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

Int IntCountNode::needle() const { return _needle; }

void IntCountNode::registerOutputVars() {
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::IfThenElseConst>(
            solver(), invariantGraph().varId(staticInputVarNodeIds().front()),
            _offset + 1, _offset, needle()));
  } else if (!staticInputVarNodeIds().empty()) {
    if (_offset == 0) {
      makeSolverVar(outputVarNodeIds().front());
    } else if (_intermediate == propagation::NULL_ID) {
      _intermediate = solver().makeIntVar(0, 0, 0);
      invariantGraph()
          .varNode(outputVarNodeIds().front())
          .setVarId(solver().makeIntView<propagation::IntOffsetView>(
              solver(), _intermediate, _offset));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntCountNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(_intermediate == propagation::NULL_ID
             ? invariantGraph().varId(outputVarNodeIds().front()).isVar()
             : invariantGraph().varId(outputVarNodeIds().front()).isView());
  assert(_intermediate == propagation::NULL_ID || _intermediate.isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());

  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars), [&](const VarNodeId node) {
                   return invariantGraph().varId(node);
                 });

  solver().makeInvariant<propagation::CountConst>(
      solver(),
      _intermediate == propagation::NULL_ID
          ? invariantGraph().varId(outputVarNodeIds().front())
          : _intermediate,
      needle(), std::move(solverVars));
}

std::string IntCountNode::dotLangIdentifier() const {
  return "int_count " + std::to_string(_needle);
}

}  // namespace atlantis::invariantgraph

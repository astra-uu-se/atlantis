#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/propagation/invariants/count.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

VarIntCountNode::VarIntCountNode(IInvariantGraph& graph,
                                 std::vector<VarNodeId>&& vars,
                                 VarNodeId needle, VarNodeId count)
    : InvariantNode(graph, {count}, append(std::move(vars), needle)) {}

void VarIntCountNode::init(InvariantNodeId id) {
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

std::vector<VarNodeId> VarIntCountNode::haystack() const {
  std::vector<VarNodeId> inputVarNodeIds;
  inputVarNodeIds.reserve(staticInputVarNodeIds().size() - 1);
  std::copy(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end() - 1,
            std::back_inserter(inputVarNodeIds));
  return inputVarNodeIds;
}

VarNodeId VarIntCountNode::needle() const {
  return staticInputVarNodeIds().back();
}

bool VarIntCountNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         invariantGraphConst().varNodeConst(needle()).isFixed();
}

bool VarIntCountNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }

  invariantGraph().addInvariantNode(std::make_shared<IntCountNode>(
      invariantGraph(), haystack(),
      invariantGraphConst().varNodeConst(needle()).lowerBound(),
      outputVarNodeIds().front()));

  return true;
}

void VarIntCountNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front());
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void VarIntCountNode::registerNode() {
  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  std::vector<VarNodeId> h = haystack();
  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(h.size());

  std::transform(
      h.begin(), h.end(), std::back_inserter(solverVars),
      [&](const VarNodeId node) { return invariantGraph().varId(node); });

  solver().makeInvariant<propagation::Count>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(needle()), std::move(solverVars));
}

std::string VarIntCountNode::dotLangIdentifier() const {
  return "var_int_count";
}

}  // namespace atlantis::invariantgraph

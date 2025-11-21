#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/count.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

VarIntCountNode::VarIntCountNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& vars,
                                 VarNodeId needle, VarNodeId count)
    : InvariantNode(graph, {count}, append(std::move(vars), needle)) {}

void VarIntCountNode::init(InvariantNodeId id) {
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

void VarIntCountNode::updateState() {
  std::vector<size_t> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size() - 1);
  const VarNode& needleNode = invariantGraphConst().varNodeConst(needle());
  for (Int i = static_cast<Int>(staticInputVarNodeIds().size()) - 2; i >= 0;
       --i) {
    const auto& vNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(i));
    if (vNode.constDomain()->isDisjoint(*needleNode.constDomain())) {
      indicesToRemove.emplace_back(i);
    }
  }
  for (const size_t index : indicesToRemove) {
    removeStaticInputAtIndex(index);
  }
  auto& outputNode = invariantGraph().varNode(outputVarNodeIds().back());
  const Int ub = static_cast<Int>(staticInputVarNodeIds().size()) - 1;
  outputNode.removeValuesBelow(0);
  outputNode.removeValuesAbove(ub);
  if (staticInputVarNodeIds().size() == 1) {
    setState(InvariantNodeState::SUBSUMED);
  }
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

void VarIntCountNode::registerOutputVars(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void VarIntCountNode::registerNode(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());

  std::vector<VarNodeId> h = haystack();
  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(h.size());

  std::ranges::transform(
      h, std::back_inserter(solverVars),
      [&](const VarNodeId node) { return mapping.solverId(node); });

  solver.makeInvariant<propagation::Count>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(needle()), std::move(solverVars));
}

std::string VarIntCountNode::dotLangIdentifier() const {
  return "var_int_count";
}

}  // namespace atlantis::invariantgraph

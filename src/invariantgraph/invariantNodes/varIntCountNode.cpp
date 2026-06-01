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
                                 const VarNodeId needle, const VarNodeId count)
    : InvariantNode(graph, {count}, append(std::move(vars), needle)) {}

void VarIntCountNode::init(const InvariantNodeId id) {
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

void VarIntCountNode::postConstraint() {
  InvariantNode::postConstraint();
  std::vector<ConstraintVarId> inputs(staticInputVarNodeIds().size() - 1, ConstraintVarId{NULL_NODE_ID});
  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()) - 1; ++i) {
    inputs[i] = staticInputVarNodeConst(i).constraintVarId();
  }
  constraintSolver().fzn_count(inputs, varNodeConst(needle()).constraintVarId(), outputVarNodeConst(0).constraintVarId(), true, RelationType::REL_TYPE_EQ);
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
  const VarNode& needleNode = varNodeConst(needle());
  for (Int i = static_cast<Int>(staticInputVarNodeIds().size()) - 2; i >= 0;
       --i) {
    const auto& vNode = staticInputVarNodeConst(i);
    if (vNode.isFixed() || vNode.constDomain()->isDisjoint(*needleNode.constDomain())) {
      indicesToRemove.emplace_back(i);
    }
  }
  for (const size_t index : indicesToRemove) {
    removeStaticInputAtIndex(index);
  }
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

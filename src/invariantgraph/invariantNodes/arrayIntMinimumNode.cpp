#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/countRelNode.hpp"
#include "atlantis/propagation/invariants/min.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intMinView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(InvariantGraph& graph,
                                         const VarNodeId a, const VarNodeId b,
                                         const VarNodeId output)
    : ArrayIntMinimumNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMinimumNode::ArrayIntMinimumNode(InvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         const VarNodeId output)
    : InvariantNode(graph, {output}, std::move(vars)) {}

void ArrayIntMinimumNode::init(const InvariantNodeId id) {
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

void ArrayIntMinimumNode::postConstraint() {
  constraintSolver().array_int_minimum(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      outputVarNode(0).constraintVarId());
}

void ArrayIntMinimumNode::updateState() {
  InvariantNode::updateState();
  const auto duplicateIndices =
      duplicateVarNodeIndices(staticInputVarNodeIds());
  for (Int i = static_cast<Int>(duplicateIndices->size() - 1); i >= 0; --i) {
    removeStaticInputAtIndex(i);
  }

  const Int lb = outputVarNodeConst(0).lowerBound();
  const Int ub = outputVarNodeConst(0).upperBound();
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  std::optional<VarNodeId> equalityVarNodeId{std::nullopt};
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    if (staticInputVarNodeConst(i).isFixed()) {
      if (staticInputVarNodeConst(i).lowerBound() == lb) {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      removeStaticInputVarNode(staticInputVarNodeIds().at(i));
    } else if (staticInputVarNodeConst(i).lowerBound() > ub) {
      removeStaticInputVarNode(staticInputVarNodeIds().at(i));
    } else if (staticInputVarNodeConst(i).lowerBound() == ub) {
      equalityVarNodeId = equalityVarNodeId.has_value()
                              ? NULL_NODE_ID
                              : staticInputVarNodeIds().at(i);
    }
  }
  if (equalityVarNodeId.has_value() && *equalityVarNodeId != NULL_NODE_ID) {
    while (staticInputVarNodeIds().size() > 1) {
      const size_t index =
          staticInputVarNodeIds().front() == *equalityVarNodeId ? 1 : 0;
      removeStaticInputAtIndex(index);
    }
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMinimumNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (staticInputVarNodeIds().size() == 1 ||
          outputVarNodeConst(0).isFixed());
}

bool ArrayIntMinimumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
    return true;
  }
  assert(outputVarNodeIds().size() == 1 && outputVarNodeConst(0).isFixed());
  invariantGraph().addInvariantNode(std::make_shared<CountRelNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
      outputVarNodeConst(0).upperBound(), Int{1}, RelationType::REL_TYPE_LE,
      true));
  return true;
}

void ArrayIntMinimumNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  assert(staticInputVarNodeIds().size() > 1);
  makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void ArrayIntMinimumNode::registerNode(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars),
      [&](const auto& node) { return mapping.solverId(node); });

  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());
  solver.makeInvariant<propagation::Min>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      std::move(solverVars));
}

std::string ArrayIntMinimumNode::dotLangIdentifier() const { return "min"; }

}  // namespace atlantis::invariantgraph

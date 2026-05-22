#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/max.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intMaxView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(InvariantGraph& graph, VarNodeId a,
                                         VarNodeId b, VarNodeId output)
    : ArrayIntMaximumNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMaximumNode::ArrayIntMaximumNode(InvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode(graph, {output}, std::move(vars)),
      _lb{std::numeric_limits<Int>::min()} {}

void ArrayIntMaximumNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(outputVarNode(0).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void ArrayIntMaximumNode::postConstraint() {
  constraintSolver().array_int_maximum(toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
                                       outputVarNode(0).constraintVarId());
}

void ArrayIntMaximumNode::updateState() {
  _lb = outputVarNodeConst(0).lowerBound();
  for (size_t i = 0; i < staticInputVarNodeIds().size();) {
    if (staticInputVarNodeConst(i).isFixed() ||
        staticInputVarNodeConst(i).upperBound() <= _lb) {
      removeStaticInputVarNode(staticInputVarNodeIds().at(i));
    } else {
      ++i;
    }
  }

  if (outputVarNodeConst(0).isFixed()) {
    for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
      staticInputVarNode(i).tightenDomainType(DomainType::DOM_UPPER_BOUND);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMaximumNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 1 &&
         _lb <= staticInputVarNodeConst(0).lowerBound();
}

bool ArrayIntMaximumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  return true;
}

void ArrayIntMaximumNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::IntMaxView>(
            solver, mapping.solverId(staticInputVarNodeIds().front()), _lb));
  } else if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void ArrayIntMaximumNode::registerNode(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(solverVars),
      [&](const auto& node) { return mapping.solverId(node); });

  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());
  solver.makeInvariant<propagation::Max>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      std::move(solverVars));
}

std::string ArrayIntMaximumNode::dotLangIdentifier() const { return "max"; }

}  // namespace atlantis::invariantgraph

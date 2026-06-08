#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/countRelNode.hpp"
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
    : InvariantNode(graph, {output}, std::move(vars)) {}

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
  constraintSolver().array_int_maximum(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      outputVarNode(0).constraintVarId());
}

void ArrayIntMaximumNode::updateState() {
  for (size_t i = 0; i < staticInputVarNodeIds().size();) {
    if (staticInputVarNodeConst(i).isFixed() ||
        staticInputVarNodeConst(i).upperBound() <
            outputVarNodeConst(0).lowerBound()) {
      removeStaticInputVarNode(staticInputVarNodeIds().at(i));
    } else {
      ++i;
    }
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMaximumNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         ((staticInputVarNodeIds().size() == 1 &&
           outputVarNodeConst(0).lowerBound() <=
               staticInputVarNodeConst(0).lowerBound()) ||
          outputVarNodeConst(0).isFixed());
}

bool ArrayIntMaximumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1 &&
      outputVarNodeConst(0).lowerBound() <=
          staticInputVarNodeConst(0).lowerBound()) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
    return true;
  }
  assert(outputVarNodeIds().size() == 1 && outputVarNodeConst(0).isFixed());
  invariantGraph().addInvariantNode(std::make_shared<CountRelNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
      outputVarNodeConst(0).lowerBound(), Int{1}, RelationType::REL_TYPE_LE,
      true));
  return true;
}

void ArrayIntMaximumNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::IntMaxView>(
            solver, mapping.solverId(staticInputVarNodeIds().front()),
            outputVarNodeConst(0).lowerBound()));
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

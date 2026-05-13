#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/min.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intMinView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(InvariantGraph& graph, VarNodeId a,
                                         VarNodeId b, VarNodeId output)
    : ArrayIntMinimumNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMinimumNode::ArrayIntMinimumNode(InvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode(graph, {output}, std::move(vars)),
      _ub(std::numeric_limits<Int>::max()) {}

void ArrayIntMinimumNode::init(InvariantNodeId id) {
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
  std::vector<ConstraintVarId> inputs(staticInputVarNodeIds().size(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    inputs[i] = staticInputVarNode(i).constraintVarId();
  }
  constraintSolver().array_int_minimum(inputs,
                                       outputVarNode(0).constraintVarId());
}

void ArrayIntMinimumNode::updateState() {
  _ub = outputVarNodeConst(0).upperBound();
  for (size_t i = 0; i < staticInputVarNodeIds().size();) {
    if (staticInputVarNodeConst(i).isFixed() ||
        _ub <= staticInputVarNodeConst(i).lowerBound()) {
      removeStaticInputVarNode(staticInputVarNodeIds().at(i));
    } else {
      ++i;
    }
  }

  if (outputVarNodeConst(0).isFixed()) {
    for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
      staticInputVarNode(i).tightenDomainType(DomainType::DOM_LOWER_BOUND);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMinimumNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 1 &&
         staticInputVarNodeConst(0).upperBound() <= _ub;
}

bool ArrayIntMinimumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  return true;
}

void ArrayIntMinimumNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() == 1) {
    mapping.setSolverId(
        outputVarNodeIds().front(),
        solver.makeIntView<propagation::IntMinView>(
            solver, mapping.solverId(staticInputVarNodeIds().front()), _ub));
  } else if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
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

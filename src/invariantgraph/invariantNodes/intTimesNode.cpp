#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/times.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

IntTimesNode::IntTimesNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                           VarNodeId output)
    : InvariantNode(graph, {output}, {a, b}), _scalar(std::nullopt) {}

void IntTimesNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isIntVar(); }));
}
void IntTimesNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_times(staticInputVarNodeConst(0).constraintVarId(),
                               staticInputVarNodeConst(1).constraintVarId(),
                               outputVarNodeConst(0).constraintVarId());
}

void IntTimesNode::updateState() {
  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& varNodeId : staticInputVarNodeIds()) {
    if (varNodeConst(varNodeId).isFixed()) {
      varNodeIdsToRemove.emplace_back(varNodeId);
      if (_scalar.has_value()) {
        _scalar = std::nullopt;
      } else {
        _scalar = varNodeConst(varNodeId).lowerBound();
      }
    }
  }

  if (_scalar == 0) {
    const auto& oNode = outputVarNodeConst(0);
    assert(oNode.isFixed() && oNode.lowerBound() == 0);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& varNodeId : varNodeIdsToRemove) {
    removeStaticInputVarNode(varNodeId);
  }

  if (staticInputVarNodeIds().empty()) {
    assert(outputVarNodeConst(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntTimesNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && _scalar.has_value() &&
         staticInputVarNodeIds().size() == 1;
}

bool IntTimesNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (_scalar.value_or(1) == 1) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
  }
  invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(
      invariantGraph(), staticInputVarNodeIds().front(),
      outputVarNodeIds().front(), *_scalar, 0));
  return true;
}

void IntTimesNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  assert(staticInputVarNodeIds().size() == 2);
  makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntTimesNode::registerNode(propagation::SolverBase& solver,
                                SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);

  assert(mapping.solverId(outputVarNodeIds().front()).isVar());

  solver.makeInvariant<propagation::Times>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(staticInputVarNodeIds().front()),
      mapping.solverId(staticInputVarNodeIds().back()));
}

std::string IntTimesNode::dotLangIdentifier() const { return "int_times"; }

}  // namespace atlantis::invariantgraph

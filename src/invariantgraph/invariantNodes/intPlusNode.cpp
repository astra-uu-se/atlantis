#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/plus.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

IntPlusNode::IntPlusNode(InvariantGraph& graph, const VarNodeId a,
                         const VarNodeId b, const VarNodeId output)
    : InvariantNode(graph, {output}, {a, b}) {}

void IntPlusNode::init(const InvariantNodeId id) {
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
void IntPlusNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_plus(staticInputVarNodeConst(0).constraintVarId(),
                              staticInputVarNodeConst(1).constraintVarId(),
                              outputVarNodeConst(0).constraintVarId());
}

void IntPlusNode::updateState() {
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (invariantGraphConst().varNodeConst(input).isFixed()) {
      varsToRemove.emplace_back(input);
      _offset += invariantGraphConst().varNodeConst(input).lowerBound();
    }
  }

  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(input);
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntPlusNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 1;
}

bool IntPlusNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(staticInputVarNodeIds().size() == 1);
  if (_offset == 0) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
    return true;
  }
  invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(
      invariantGraph(), staticInputVarNodeIds().front(),
      outputVarNodeIds().front(), 1, _offset));
  return true;
}

void IntPlusNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  assert(staticInputVarNodeIds().size() == 2);
  makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntPlusNode::registerNode(propagation::SolverBase& solver,
                               SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(_offset == 0);
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());

  solver.makeInvariant<propagation::Plus>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(staticInputVarNodeIds().front()),
      mapping.solverId(staticInputVarNodeIds().back()));
}

std::string IntPlusNode::dotLangIdentifier() const { return "int_plus"; }

}  // namespace atlantis::invariantgraph

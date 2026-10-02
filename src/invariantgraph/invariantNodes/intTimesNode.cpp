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

IntTimesNode::IntTimesNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                           VarNode& output)
    : InvariantNode(graph, {output}, {a, b}), _scalar(std::nullopt) {}

void IntTimesNode::init() {
  InvariantNode::init();
  assert(
      invariantGraphConst().varNodeConst(outputVarNodes().front()).isIntVar());
  assert(std::ranges::all_of(staticInputVarNodes().begin(),
                             staticInputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return varNodeConst(vId).isIntVar();
                             }));
}

void IntTimesNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_times(staticInputVarNodeConst(0).constraintVarId(),
                               staticInputVarNodeConst(1).constraintVarId(),
                               outputVarNodeConst(0).constraintVarId());
}

void IntTimesNode::updateState() {
  std::vector<std::shared_ptr<VarNode>> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(staticInputVarNodes().size());

  for (const auto& varNodeId : staticInputVarNodes()) {
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

  if (staticInputVarNodes().empty()) {
    assert(outputVarNodeConst(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntTimesNode::constrainsOutput(VarNode&) const {
  const auto extremums =
      std::array<Int, 4>{staticInputVarNodeConst(0).lowerBound() *
                             staticInputVarNodeConst(1).lowerBound(),
                         staticInputVarNodeConst(0).lowerBound() *
                             staticInputVarNodeConst(1).upperBound(),
                         staticInputVarNodeConst(0).upperBound() *
                             staticInputVarNodeConst(1).lowerBound(),
                         staticInputVarNodeConst(0).upperBound() *
                             staticInputVarNodeConst(1).upperBound()};
  return !outputVarNodeConst(0).constDomain()->contains(
      std::ranges::min(extremums), std::ranges::max(extremums));
}

bool IntTimesNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && _scalar.has_value() &&
         staticInputVarNodes().size() == 1;
}

bool IntTimesNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (_scalar.value_or(1) == 1) {
    invariantGraph().replaceVarNode(outputVarNodes().front(),
                                    staticInputVarNodes().front());
  }
  invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(
      invariantGraph(), staticInputVarNodes().front(), outputVarNodes().front(),
      *_scalar, 0));
  return true;
}

void IntTimesNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  assert(staticInputVarNodes().size() == 2);
  makeSolverVar(outputVarNodes().front(), solver, mapping);
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vId) {
                               return mapping.solverId(vId) !=
                                      propagation::NULL_ID;
                             }));
}

void IntTimesNode::registerNode(propagation::SolverBase& solver,
                                SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNodes().front()) != propagation::NULL_ID);

  assert(mapping.solverId(outputVarNodes().front()).isVar());

  solver.makeInvariant<propagation::Times>(
      solver, mapping.solverId(outputVarNodes().front()),
      mapping.solverId(staticInputVarNodes().front()),
      mapping.solverId(staticInputVarNodes().back()));
}

std::string IntTimesNode::dotLangIdentifier() const { return "int_times"; }

}  // namespace atlantis::invariantgraph

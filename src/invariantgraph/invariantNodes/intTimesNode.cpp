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
    : InvariantNode(graph, {output.ptr()}, {a.ptr(), b.ptr()}), _scalar(std::nullopt) {}

void IntTimesNode::init() {
  InvariantNode::init();
  assert(
      outputVarNode(0).isIntVar());
  assert(std::ranges::all_of(staticInputVarNodes().begin(),
                             staticInputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return vNode->isIntVar();
                             }));
}

void IntTimesNode::postConstraint() {
  InvariantNode::postConstraint();
  constraintSolver().int_times(staticInputVarNode(0).constraintVarId(),
                               staticInputVarNode(1).constraintVarId(),
                               outputVarNode(0).constraintVarId());
}

void IntTimesNode::updateState() {
  std::vector<std::shared_ptr<VarNode>> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(staticInputVarNodes().size());

  for (const auto& vNode : staticInputVarNodes()) {
    if (vNode->isFixed()) {
      varNodeIdsToRemove.emplace_back(vNode);
      if (_scalar.has_value()) {
        _scalar = std::nullopt;
      } else {
        _scalar = vNode->lowerBound();
      }
    }
  }

  if (_scalar == 0) {
    const auto& oNode = outputVarNode(0);
    assert(oNode.isFixed() && oNode.lowerBound() == 0);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& vNode : varNodeIdsToRemove) {
    removeStaticInputVarNode(*vNode);
  }

  if (staticInputVarNodes().empty()) {
    assert(outputVarNode(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntTimesNode::constrainsOutput(const VarNode&) const {
  const auto extremums =
      std::array<Int, 4>{staticInputVarNode(0).lowerBound() *
                             staticInputVarNode(1).lowerBound(),
                         staticInputVarNode(0).lowerBound() *
                             staticInputVarNode(1).upperBound(),
                         staticInputVarNode(0).upperBound() *
                             staticInputVarNode(1).lowerBound(),
                         staticInputVarNode(0).upperBound() *
                             staticInputVarNode(1).upperBound()};
  return !outputVarNode(0).constDomain()->contains(
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
    invariantGraph().replaceVarNode(outputVarNode(0),
                                    staticInputVarNode(0));
  }
  invariantGraph().addInvariantNode(std::make_shared<IntScalarNode>(
      invariantGraph(), staticInputVarNode(0), outputVarNode(0),
      *_scalar, 0));
  return true;
}

void IntTimesNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  assert(staticInputVarNodes().size() == 2);
  makeSolverVar(outputVarNode(0), solver, mapping);
  assert(std::ranges::all_of(outputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return mapping.solverId(vNode) !=
                                      propagation::NULL_ID;
                             }));
}

void IntTimesNode::registerNode(propagation::SolverBase& solver,
                                SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  assert(mapping.solverId(outputVarNode(0)) != propagation::NULL_ID);

  assert(mapping.solverId(outputVarNode(0)).isVar());

  solver.makeInvariant<propagation::Times>(
      solver, mapping.solverId(outputVarNode(0)),
      mapping.solverId(staticInputVarNode(0)),
      mapping.solverId(staticInputVarNodes().back()));
}

std::string IntTimesNode::dotLangIdentifier() const { return "int_times"; }

}  // namespace atlantis::invariantgraph

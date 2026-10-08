#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

namespace atlantis::invariantgraph {

BoolClauseNode::BoolClauseNode(InvariantGraph& graph,
                               std::vector<std::shared_ptr<VarNode>>&& posVars,
                               std::vector<std::shared_ptr<VarNode>>&& negVars,
                               VarNode& r)
    : ViolationInvariantNode(graph, concat(posVars, negVars), r),
      _numPosVars(posVars.size()) {}
BoolClauseNode::BoolClauseNode(InvariantGraph& graph,
                               std::vector<std::shared_ptr<VarNode>>&& posVars,
                               std::vector<std::shared_ptr<VarNode>>&& negVars,
                               const bool shouldHold)
    : ViolationInvariantNode(graph, concat(posVars, negVars), shouldHold),
      _numPosVars(posVars.size()) {}

void BoolClauseNode::init() {
  ViolationInvariantNode::init();
  assert(
      !isReified() ||
      !reifiedViolationNode()->isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void BoolClauseNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  std::vector<ConstraintVarId> posVars(_numPosVars,
                                       ConstraintVarId{NULL_NODE_ID});
  std::vector<ConstraintVarId> negVars(
      staticInputVarNodes().size() - _numPosVars,
      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < _numPosVars; i++) {
    posVars[i] = staticInputVarNode(i).constraintVarId();
  }
  for (size_t i = _numPosVars; i < staticInputVarNodes().size(); ++i) {
    negVars[i - _numPosVars] = staticInputVarNode(i).constraintVarId();
  }
  if (isReified()) {
    constraintSolver().bool_clause_reif(
        posVars, negVars, reifiedViolationNode()->constraintVarId());
  } else {
    constraintSolver().bool_clause(posVars, negVars, shouldHold());
  }
}

void BoolClauseNode::updateState() {
  ViolationInvariantNode::updateState();
  for (size_t i = 0; i < _numPosVars; ++i) {
    for (size_t j = _numPosVars; j < staticInputVarNodes().size(); ++j) {
      if (&staticInputVarNode(i) == &staticInputVarNode(j)) {
        if (isReified()) {
          fixReified(true);
        } else if (!shouldHold()) {
          throw InconsistencyException(
              "BoolClauseNode::updateState constraint is violated");
        }
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
    }
  }

  std::vector<std::shared_ptr<VarNode>> varsToRemove;
  varsToRemove.reserve(staticInputVarNodes().size());
  size_t numPosRemoved = 0;

  for (size_t i = 0; i < _numPosVars; ++i) {
    if (staticInputVarNode(i).isFixed()) {
      if (staticInputVarNode(i).inDomain(bool{true})) {
        assert(!isReified());
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      varsToRemove.emplace_back(staticInputVarNodes().at(i));
      ++numPosRemoved;
    }
  }

  for (size_t i = _numPosVars; i < staticInputVarNodes().size(); ++i) {
    if (staticInputVarNode(i).isFixed()) {
      if (staticInputVarNode(i).inDomain(bool{false})) {
        assert(!isReified());
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      varsToRemove.emplace_back(staticInputVarNodes().at(i));
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(*input);
  }
  _numPosVars -= numPosRemoved;
  if (staticInputVarNodes().empty()) {
    assert(!isReified());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolClauseNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE;
}

bool BoolClauseNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodes().empty()) {
    return true;
  }
  if (staticInputVarNodes().size() == 1) {
    if (isReified()) {
      if (_numPosVars > 0) {
        invariantGraph().replaceVarNode(*reifiedViolationNode(),
                                        staticInputVarNode(0));
      } else {
        invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
            invariantGraph(), staticInputVarNode(0),
            *reifiedViolationNode()));
      }
    } else {
      staticInputVarNode(0).fixToValue(_numPosVars > 0 ? shouldHold() : !shouldHold());
    }
    return true;
  }

  std::vector<std::shared_ptr<VarNode>> boolOrInputs;
  boolOrInputs.reserve(staticInputVarNodes().size());
  for (size_t i = 0; i < _numPosVars; ++i) {
    boolOrInputs.emplace_back(staticInputVarNodes().at(i));
  }
  for (size_t i = _numPosVars; i < staticInputVarNodes().size(); ++i) {
    boolOrInputs.emplace_back(invariantGraph().retrieveBoolVarNode().ptr());
    invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
        invariantGraph(), staticInputVarNode(i), *boolOrInputs.back()));
  }

  if (isReified()) {
    invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
        invariantGraph(), std::move(boolOrInputs), *reifiedViolationNode()));
  } else {
    invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
        invariantGraph(), std::move(boolOrInputs), shouldHold()));
  }
  return true;
}

void BoolClauseNode::registerOutputVars(propagation::SolverBase&,
                                        SolverMapping&) const {
  throw std::runtime_error(
      "BoolClauseNode::registerOutputVars not implemented");
}

void BoolClauseNode::registerNode(propagation::SolverBase&,
                                  SolverMapping&) const {
  throw std::runtime_error("BoolClauseNode::registerNode not implemented");
}

std::string BoolClauseNode::dotLangIdentifier() const { return "bool_clause"; }

}  // namespace atlantis::invariantgraph

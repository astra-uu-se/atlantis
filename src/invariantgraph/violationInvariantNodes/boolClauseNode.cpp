#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

namespace atlantis::invariantgraph {

BoolClauseNode::BoolClauseNode(InvariantGraph& graph,
                               std::vector<VarNodeId>&& posVars,
                               std::vector<VarNodeId>&& negVars,
                               const VarNodeId r)
    : ViolationInvariantNode(graph, concat(posVars, negVars), r),
      _numPosVars(posVars.size()) {}
BoolClauseNode::BoolClauseNode(InvariantGraph& graph,
                               std::vector<VarNodeId>&& posVars,
                               std::vector<VarNodeId>&& negVars,
                               const bool shouldHold)
    : ViolationInvariantNode(graph, concat(posVars, negVars), shouldHold),
      _numPosVars(posVars.size()) {}

void BoolClauseNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BoolClauseNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  std::vector<ConstraintVarId> posVars(_numPosVars,
                                       ConstraintVarId{NULL_NODE_ID});
  std::vector<ConstraintVarId> negVars(
      staticInputVarNodeIds().size() - _numPosVars,
      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < _numPosVars; i++) {
    posVars[i] = staticInputVarNodeConst(i).constraintVarId();
  }
  for (size_t i = _numPosVars; i < staticInputVarNodeIds().size(); ++i) {
    negVars[i - _numPosVars] = staticInputVarNodeConst(i).constraintVarId();
  }
  if (isReified()) {
    constraintSolver().bool_clause_reif(
        posVars, negVars, reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_clause(posVars, negVars, shouldHold());
  }
}

void BoolClauseNode::updateState() {
  ViolationInvariantNode::updateState();
  for (size_t i = 0; i < _numPosVars; ++i) {
    for (size_t j = _numPosVars; j < staticInputVarNodeIds().size(); ++j) {
      if (staticInputVarNodeIds().at(i) == staticInputVarNodeIds().at(j)) {
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

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  size_t numPosRemoved = 0;

  for (size_t i = 0; i < _numPosVars; ++i) {
    if (staticInputVarNodeConst(i).isFixed()) {
      if (staticInputVarNodeConst(i).inDomain(bool{true})) {
        assert(!isReified());
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      varsToRemove.emplace_back(staticInputVarNodeIds().at(i));
      ++numPosRemoved;
    }
  }

  for (size_t i = _numPosVars; i < staticInputVarNodeIds().size(); ++i) {
    if (staticInputVarNodeConst(i).isFixed()) {
      if (staticInputVarNodeConst(i).inDomain(bool{false})) {
        assert(!isReified());
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      varsToRemove.emplace_back(staticInputVarNodeIds().at(i));
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(input);
  }
  _numPosVars -= numPosRemoved;
  if (staticInputVarNodeIds().empty()) {
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
  if (staticInputVarNodeIds().empty()) {
    return true;
  }
  if (staticInputVarNodeIds().size() == 1) {
    if (isReified()) {
      if (_numPosVars > 0) {
        invariantGraph().replaceVarNode(reifiedViolationNodeId(),
                                        staticInputVarNodeIds().front());
      } else {
        invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
            invariantGraph(), staticInputVarNodeIds().front(),
            reifiedViolationNodeId()));
      }
    } else {
      invariantGraph()
          .varNode(staticInputVarNodeIds().front())
          .fixToValue(_numPosVars > 0 ? shouldHold() : !shouldHold());
    }
    return true;
  }

  std::vector<VarNodeId> boolOrInputs;
  boolOrInputs.reserve(staticInputVarNodeIds().size());
  for (size_t i = 0; i < _numPosVars; ++i) {
    boolOrInputs.emplace_back(staticInputVarNodeIds().at(i));
  }
  for (size_t i = _numPosVars; i < staticInputVarNodeIds().size(); ++i) {
    boolOrInputs.emplace_back(invariantGraph().retrieveBoolVarNode());
    invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
        invariantGraph(), staticInputVarNodeIds().at(i), boolOrInputs.back()));
  }

  if (isReified()) {
    invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
        invariantGraph(), std::move(boolOrInputs), reifiedViolationNodeId()));
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

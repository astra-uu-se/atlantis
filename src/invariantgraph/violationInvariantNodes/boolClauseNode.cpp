#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

BoolClauseNode::BoolClauseNode(IInvariantGraph& graph,
                               std::vector<VarNodeId>&& as,
                               std::vector<VarNodeId>&& bs, VarNodeId r)
    : ViolationInvariantNode(graph, concat(as, bs), r), _numAs(as.size()) {}
BoolClauseNode::BoolClauseNode(IInvariantGraph& graph,
                               std::vector<VarNodeId>&& as,
                               std::vector<VarNodeId>&& bs, bool shouldHold)
    : ViolationInvariantNode(graph, concat(as, bs), shouldHold),
      _numAs(as.size()) {}

void BoolClauseNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::none_of(staticInputVarNodeIds().begin(),
                   staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                     return invariantGraphConst().varNodeConst(vId).isIntVar();
                   }));
}

void BoolClauseNode::updateState() {
  ViolationInvariantNode::updateState();
  for (size_t i = 0; i < _numAs; ++i) {
    for (size_t j = _numAs; j < staticInputVarNodeIds().size(); ++j) {
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
  size_t numAsRemoved = 0;

  for (size_t i = 0; i < _numAs; ++i) {
    if (invariantGraph()
            .varNodeConst(staticInputVarNodeIds().at(i))
            .isFixed()) {
      if (invariantGraph()
              .varNodeConst(staticInputVarNodeIds().at(i))
              .inDomain(bool{true})) {
        if (isReified()) {
          fixReified(true);
        } else if (!shouldHold()) {
          throw InconsistencyException(
              "BoolClauseNode::updateState constraint is violated");
        }
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      varsToRemove.emplace_back(staticInputVarNodeIds().at(i));
      ++numAsRemoved;
    }
  }

  for (size_t i = _numAs; i < staticInputVarNodeIds().size(); ++i) {
    if (invariantGraph()
            .varNodeConst(staticInputVarNodeIds().at(i))
            .isFixed()) {
      if (invariantGraph()
              .varNodeConst(staticInputVarNodeIds().at(i))
              .inDomain(bool{false})) {
        if (isReified()) {
          fixReified(true);
        } else if (!shouldHold()) {
          throw InconsistencyException(
              "BoolClauseNode::updateState constraint is violated");
        }
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      varsToRemove.emplace_back(staticInputVarNodeIds().at(i));
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(input);
  }
  _numAs -= numAsRemoved;
  if (staticInputVarNodeIds().empty()) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException(
          "BoolClauseNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (staticInputVarNodeIds().size() == 1 && !isReified()) {
    auto& inputNode = invariantGraph().varNode(staticInputVarNodeIds().front());
    inputNode.fixToValue(_numAs > 0 ? shouldHold() : !shouldHold());
    removeStaticInputVarNode(inputNode.varNodeId());
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
      if (_numAs > 0) {
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
          .fixToValue(_numAs > 0 ? shouldHold() : !shouldHold());
    }
    return true;
  }

  std::vector<VarNodeId> boolOrInputs;
  boolOrInputs.reserve(staticInputVarNodeIds().size());
  for (size_t i = 0; i < _numAs; ++i) {
    boolOrInputs.emplace_back(staticInputVarNodeIds().at(i));
  }
  for (size_t i = _numAs; i < staticInputVarNodeIds().size(); ++i) {
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

void BoolClauseNode::registerOutputVars() {
  throw std::runtime_error(
      "BoolClauseNode::registerOutputVars not implemented");
}

void BoolClauseNode::registerNode() {
  throw std::runtime_error("BoolClauseNode::registerNode not implemented");
}

std::string BoolClauseNode::dotLangIdentifier() const { return "bool_clause"; }

}  // namespace atlantis::invariantgraph

#include "atlantis/invariantgraph/invariantNode.hpp"

#include <cassert>
#include <fznparser/model.hpp>
#include <unordered_map>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"

namespace atlantis::invariantgraph {
/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a soft constraint (which defines a violation), or a view.
 */
InvariantNode::InvariantNode(std::vector<VarNodeId>&& outputIds,
                             std::vector<VarNodeId>&& staticInputIds,
                             std::vector<VarNodeId>&& dynamicInputIds)
    : _outputVarNodeIds(std::move(outputIds)),
      _staticInputVarNodeIds(std::move(staticInputIds)),
      _dynamicInputVarNodeIds(std::move(dynamicInputIds)) {}

bool InvariantNode::isReified() const { return false; }

bool InvariantNode::canBeReplaced(const InvariantGraph&) const { return false; }

bool InvariantNode::replace(InvariantGraph&) { return false; }

bool InvariantNode::canBeMadeImplicit(const InvariantGraph&) const {
  return false;
}

bool InvariantNode::makeImplicit(InvariantGraph&) { return false; }

void InvariantNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  if (_state != InvariantNodeState::UNINITIALIZED) {
    return;
  }
  assert(_id == NULL_NODE_ID);
  _id = id;
  for (size_t i = 0; i < _outputVarNodeIds.size(); ++i) {
    markOutputTo(graph.varNode(_outputVarNodeIds[i]), false);
  }
  for (VarNodeId varNodeId : _outputVarNodeIds) {
    markOutputTo(graph.varNode(varNodeId), false);
  }
  for (VarNodeId varNodeId : _staticInputVarNodeIds) {
    markStaticInputTo(graph.varNode(varNodeId), false);
  }
  for (VarNodeId varNodeId : _dynamicInputVarNodeIds) {
    markDynamicInputTo(graph.varNode(varNodeId), false);
  }
  _state = InvariantNodeState::ACTIVE;
}

void InvariantNode::deactivate(InvariantGraph& graph) {
  while (!staticInputVarNodeIds().empty()) {
    removeStaticInputVarNode(graph.varNode(staticInputVarNodeIds().front()));
  }
  while (!dynamicInputVarNodeIds().empty()) {
    removeDynamicInputVarNode(graph.varNode(dynamicInputVarNodeIds().front()));
  }
  while (!outputVarNodeIds().empty()) {
    removeOutputVarNode(graph.varNode(outputVarNodeIds().front()));
  }
  _state = InvariantNodeState::SUBSUMED;
}

/**
 * @return The variable nodes defined by this node.
 */
const std::vector<VarNodeId>& InvariantNode::outputVarNodeIds() const noexcept {
  return _outputVarNodeIds;
}

/**
 * @return The violation variable of this variable defining node. Only
 * applicable if the current node is a soft constraint. If this node does not
 * define a violation variable, this method returns @p nullptr.
 */
propagation::VarId InvariantNode::violationVarId(const InvariantGraph&) const {
  return propagation::NULL_ID;
}

const std::vector<VarNodeId>& InvariantNode::staticInputVarNodeIds()
    const noexcept {
  return _staticInputVarNodeIds;
}

const std::vector<VarNodeId>& InvariantNode::dynamicInputVarNodeIds()
    const noexcept {
  return _dynamicInputVarNodeIds;
}

void InvariantNode::replaceDefinedVar(VarNode& oldOutputVarNode,
                                      VarNode& newOutputVarNode) {
  // Replace all occurrences:
  for (auto& _outputVarNodeId : _outputVarNodeIds) {
    if (_outputVarNodeId == oldOutputVarNode.varNodeId()) {
      _outputVarNodeId = newOutputVarNode.varNodeId();
    }
  }
  oldOutputVarNode.unmarkOutputTo(_id);
  newOutputVarNode.markOutputTo(_id);
}

void InvariantNode::removeStaticInputVarNode(VarNode& retrieveVarNode) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_staticInputVarNodeIds.size()) - 1; i >= 0;
       --i) {
    if (_staticInputVarNodeIds[i] == retrieveVarNode.varNodeId()) {
      _staticInputVarNodeIds.erase(_staticInputVarNodeIds.begin() + i);
    }
  }
  retrieveVarNode.unmarkAsInputFor(_id, true);
}

void InvariantNode::removeDynamicInputVarNode(VarNode& retrieveVarNode) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_dynamicInputVarNodeIds.size()) - 1; i >= 0;
       --i) {
    if (_dynamicInputVarNodeIds[i] == retrieveVarNode.varNodeId()) {
      _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.begin() + i);
    }
  }
  retrieveVarNode.unmarkAsInputFor(_id, false);
}

void InvariantNode::removeOutputVarNode(VarNode& outputVarNode) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_outputVarNodeIds.size()) - 1; i >= 0; --i) {
    if (_outputVarNodeIds[i] == outputVarNode.varNodeId()) {
      _outputVarNodeIds.erase(_outputVarNodeIds.begin() + i);
    }
  }
  outputVarNode.unmarkOutputTo(_id);
}

void InvariantNode::replaceStaticInputVarNode(VarNode& oldInputVarNode,
                                              VarNode& newInputVarNode) {
  // Replace all occurrences:
  for (size_t i = 0; i < _staticInputVarNodeIds.size(); ++i) {
    if (_staticInputVarNodeIds[i] == oldInputVarNode.varNodeId()) {
      _staticInputVarNodeIds[i] = newInputVarNode.varNodeId();
    }
  }
  oldInputVarNode.unmarkAsInputFor(_id, true);
  newInputVarNode.markAsInputFor(_id, true);
}

void InvariantNode::replaceDynamicInputVarNode(VarNode& oldInputVarNode,
                                               VarNode& newInputVarNode) {
  // Replace all occurrences:
  for (size_t i = 0; i < _dynamicInputVarNodeIds.size(); ++i) {
    if (_dynamicInputVarNodeIds[i] == oldInputVarNode.varNodeId()) {
      _dynamicInputVarNodeIds[i] = newInputVarNode.varNodeId();
    }
  }
  oldInputVarNode.unmarkAsInputFor(_id, false);
  newInputVarNode.markAsInputFor(_id, false);
}

std::ostream& InvariantNode::dotLangEntry(std::ostream& o) const {
  return o << _id << "[shape=box,label=\"" << dotLangIdentifier() << "\"];"
           << std::endl;
}

std::ostream& InvariantNode::dotLangEdges(std::ostream& o) const {
  for (const auto& vId : staticInputVarNodeIds()) {
    o << vId << " -> " << _id << "[style=solid];" << std::endl;
  }
  for (const auto& vId : dynamicInputVarNodeIds()) {
    o << vId << " -> " << _id << "[style=dashed];" << std::endl;
  }
  for (const auto& vId : outputVarNodeIds()) {
    o << _id << " -> " << vId << "[style = solid];" << std::endl;
  }
  return o;
}

void InvariantNode::eraseStaticInputVarNode(size_t index) {
  if (index >= _staticInputVarNodeIds.size()) {
    throw InvariantGraphException(
        "InvariantNode::eraseStaticInputVarNode: index out of bounds");
  }
  _staticInputVarNodeIds.erase(_staticInputVarNodeIds.begin() + index);
}

void InvariantNode::eraseDynamicInputVarNode(size_t index) {
  if (index >= _dynamicInputVarNodeIds.size()) {
    throw InvariantGraphException(
        "InvariantNode::eraseDynamicInputVarNode: index out of bounds");
  }
  _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.begin() + index);
}

std::vector<std::pair<VarNodeId, VarNodeId>> InvariantNode::splitOutputVarNodes(
    InvariantGraph& graph) {
  std::vector<std::pair<VarNodeId, VarNodeId>> replaced;
  replaced.reserve(_outputVarNodeIds.size());
  for (size_t i = 0; i < _outputVarNodeIds.size(); ++i) {
    const VarNode& iNode = graph.varNodeConst(_outputVarNodeIds[i]);
    if (iNode.isFixed()) {
      continue;
    }
    for (size_t j = i + 1; j < _outputVarNodeIds.size(); ++j) {
      const VarNode& jNode = graph.varNodeConst(_outputVarNodeIds[j]);
      if (jNode.isFixed()) {
        continue;
      }
      if (_outputVarNodeIds[i] == _outputVarNodeIds[j]) {
        _outputVarNodeIds[j] = graph.retrieveIntVarNode(
            SearchDomain{iNode.constDomain()}, iNode.domainType());
        graph.varNode(_outputVarNodeIds[j]).markOutputTo(_id);
        replaced.emplace_back(_outputVarNodeIds[i], _outputVarNodeIds[j]);
      }
    }
  }
  return replaced;
}

propagation::VarId InvariantNode::makeSolverVar(propagation::SolverBase& solver,
                                                VarNode& varNode,
                                                Int initialValue) {
  if (varNode.varId() == propagation::NULL_ID) {
    varNode.setVarId(solver.makeIntVar(
        std::max(varNode.lowerBound(),
                 std::min(varNode.upperBound(), initialValue)),
        varNode.lowerBound(), varNode.upperBound()));
  }
  return varNode.varId();
}

void InvariantNode::markOutputTo(VarNode& varNode, bool registerHere) {
  varNode.markOutputTo(_id);

  if (registerHere) {
    _outputVarNodeIds.push_back(varNode.varNodeId());
  }
}

void InvariantNode::markStaticInputTo(VarNode& varNode, bool registerHere) {
  varNode.markAsInputFor(_id, true);

  if (registerHere) {
    _staticInputVarNodeIds.push_back(varNode.varNodeId());
  }
}

void InvariantNode::markDynamicInputTo(VarNode& varNode, bool registerHere) {
  varNode.markAsInputFor(_id, false);

  if (registerHere) {
    _dynamicInputVarNodeIds.push_back(varNode.varNodeId());
  }
}
}  // namespace atlantis::invariantgraph

#include "atlantis/invariantgraph/invariantNode.hpp"

#include <cassert>
#include <fznparser/model.hpp>
#include <unordered_map>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/iInvariantGraph.hpp"

namespace atlantis::invariantgraph {
/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a soft constraint (which defines a violation), or a view.
 */
InvariantNode::InvariantNode(IInvariantGraph& invariantGraph,
                             std::vector<VarNodeId>&& outputIds,
                             std::vector<VarNodeId>&& staticInputIds,
                             std::vector<VarNodeId>&& dynamicInputIds)
    : _invariantGraph(invariantGraph),
      _outputVarNodeIds(std::move(outputIds)),
      _staticInputVarNodeIds(std::move(staticInputIds)),
      _dynamicInputVarNodeIds(std::move(dynamicInputIds)) {}

IInvariantGraph& InvariantNode::invariantGraph() { return _invariantGraph; }

void InvariantNode::setState(InvariantNodeState state) { _state = state; }

const IInvariantGraph& InvariantNode::invariantGraphConst() const {
  return _invariantGraph;
}

propagation::SolverBase& InvariantNode::solver() {
  return _invariantGraph.solver();
}

const propagation::SolverBase& InvariantNode::solverConst() const {
  return _invariantGraph.solverConst();
}

InvariantNodeId InvariantNode::id() const { return _id; }

bool InvariantNode::isReified() const { return false; }

void InvariantNode::updateState() {}

bool InvariantNode::canBeReplaced() const { return false; }

bool InvariantNode::replace() { return false; }

bool InvariantNode::canBeMadeImplicit() const { return false; }

bool InvariantNode::makeImplicit() { return false; }

InvariantNodeState InvariantNode::state() const { return _state; }

const std::vector<VarNodeId>& InvariantNode::outputVarNodeIds() const {
  return _outputVarNodeIds;
}
const std::vector<VarNodeId>& InvariantNode::staticInputVarNodeIds() const {
  return _staticInputVarNodeIds;
}
const std::vector<VarNodeId>& InvariantNode::dynamicInputVarNodeIds() const {
  return _dynamicInputVarNodeIds;
}

void InvariantNode::init(InvariantNodeId id) {
  if (_state != InvariantNodeState::UNINITIALIZED) {
    return;
  }
  assert(_id == NULL_NODE_ID);
  _id = id;
  for (VarNodeId varNodeId : _outputVarNodeIds) {
    markOutputTo(varNodeId, false);
  }
  for (VarNodeId varNodeId : _staticInputVarNodeIds) {
    markStaticInputTo(varNodeId, false);
  }
  for (VarNodeId varNodeId : _dynamicInputVarNodeIds) {
    markDynamicInputTo(varNodeId, false);
  }
  _state = InvariantNodeState::ACTIVE;
}

propagation::VarViewId InvariantNode::violationVarId() const {
  return propagation::NULL_ID;
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

void InvariantNode::deactivate() {
  while (!staticInputVarNodeIds().empty()) {
    removeStaticInputVarNode(staticInputVarNodeIds().front());
  }
  while (!dynamicInputVarNodeIds().empty()) {
    removeDynamicInputVarNode(dynamicInputVarNodeIds().front());
  }
  while (!outputVarNodeIds().empty()) {
    removeOutputVarNode(outputVarNodeIds().front());
  }
  setState(InvariantNodeState::SUBSUMED);
}

void InvariantNode::replaceDefinedVar(VarNodeId oldOutputVarNodeId,
                                      VarNodeId newOutputVarNodeId) {
  // Replace all occurrences:
  for (auto& _outputVarNodeId : _outputVarNodeIds) {
    if (_outputVarNodeId == oldOutputVarNodeId) {
      _outputVarNodeId = newOutputVarNodeId;
    }
  }
  _invariantGraph.varNode(oldOutputVarNodeId).unmarkOutputTo(_id);
  _invariantGraph.varNode(newOutputVarNodeId).markOutputTo(_id);
}

void InvariantNode::removeStaticInputVarNode(VarNodeId retrieveVarNodeId) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_staticInputVarNodeIds.size()) - 1; i >= 0;
       --i) {
    if (_staticInputVarNodeIds[i] == retrieveVarNodeId) {
      _staticInputVarNodeIds.erase(_staticInputVarNodeIds.begin() + i);
    }
  }
  _invariantGraph.varNode(retrieveVarNodeId).unmarkAsInputFor(_id, true);
}

void InvariantNode::removeDynamicInputVarNode(VarNodeId retrieveVarNodeId) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_dynamicInputVarNodeIds.size()) - 1; i >= 0;
       --i) {
    if (_dynamicInputVarNodeIds[i] == retrieveVarNodeId) {
      _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.begin() + i);
    }
  }
  _invariantGraph.varNode(retrieveVarNodeId).unmarkAsInputFor(_id, false);
}

void InvariantNode::removeOutputVarNode(VarNodeId outputVarNodeId) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_outputVarNodeIds.size()) - 1; i >= 0; --i) {
    if (_outputVarNodeIds[i] == outputVarNodeId) {
      _outputVarNodeIds.erase(_outputVarNodeIds.begin() + i);
    }
  }
  _invariantGraph.varNode(outputVarNodeId).unmarkOutputTo(_id);
}

void InvariantNode::replaceStaticInputVarNode(VarNodeId oldInputVarNodeId,
                                              VarNodeId newInputVarNodeId) {
  // Replace all occurrences:
  for (size_t i = 0; i < _staticInputVarNodeIds.size(); ++i) {
    if (_staticInputVarNodeIds[i] == oldInputVarNodeId) {
      _staticInputVarNodeIds[i] = newInputVarNodeId;
    }
  }
  _invariantGraph.varNode(oldInputVarNodeId).unmarkAsInputFor(_id, true);
  _invariantGraph.varNode(newInputVarNodeId).markAsInputFor(_id, true);
}

void InvariantNode::replaceDynamicInputVarNode(VarNodeId oldInputVarNodeId,
                                               VarNodeId newInputVarNodeId) {
  // Replace all occurrences:
  for (size_t i = 0; i < _dynamicInputVarNodeIds.size(); ++i) {
    if (_dynamicInputVarNodeIds[i] == oldInputVarNodeId) {
      _dynamicInputVarNodeIds[i] = newInputVarNodeId;
    }
  }
  _invariantGraph.varNode(oldInputVarNodeId).unmarkAsInputFor(_id, false);
  _invariantGraph.varNode(newInputVarNodeId).markAsInputFor(_id, false);
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

std::vector<std::pair<VarNodeId, VarNodeId>>
InvariantNode::splitOutputVarNodes() {
  std::vector<std::pair<VarNodeId, VarNodeId>> replaced;
  replaced.reserve(_outputVarNodeIds.size());
  for (size_t i = 0; i < _outputVarNodeIds.size(); ++i) {
    const VarNode& iNode = _invariantGraph.varNodeConst(_outputVarNodeIds[i]);
    if (iNode.isFixed()) {
      continue;
    }
    for (size_t j = i + 1; j < _outputVarNodeIds.size(); ++j) {
      const VarNode& jNode = _invariantGraph.varNodeConst(_outputVarNodeIds[j]);
      if (jNode.isFixed()) {
        continue;
      }
      if (_outputVarNodeIds[i] == _outputVarNodeIds[j]) {
        _outputVarNodeIds[j] = _invariantGraph.retrieveIntVarNode(
            SearchDomain{iNode.constDomain()}, iNode.domainType());
        _invariantGraph.varNode(_outputVarNodeIds[j]).markOutputTo(_id);
        replaced.emplace_back(_outputVarNodeIds[i], _outputVarNodeIds[j]);
      }
    }
  }
  return replaced;
}

propagation::VarViewId InvariantNode::makeSolverVar(VarNodeId varNodeId,
                                                    Int initialValue) {
  auto& varNode = _invariantGraph.varNode(varNodeId);
  if (varNode.varId() == propagation::NULL_ID) {
    varNode.setVarId(solver().makeIntVar(
        std::max(varNode.lowerBound(),
                 std::min(varNode.upperBound(), initialValue)),
        varNode.lowerBound(), varNode.upperBound()));
  }
  return varNode.varId();
}

propagation::VarViewId InvariantNode::makeSolverVar(VarNodeId varNodeId) {
  return makeSolverVar(varNodeId, 0);
}

void InvariantNode::markOutputTo(VarNodeId varNodeId, bool registerHere) {
  _invariantGraph.varNode(varNodeId).markOutputTo(_id);

  if (registerHere) {
    _outputVarNodeIds.push_back(varNodeId);
  }
}

void InvariantNode::markStaticInputTo(VarNodeId varNodeId, bool registerHere) {
  _invariantGraph.varNode(varNodeId).markAsInputFor(_id, true);

  if (registerHere) {
    _staticInputVarNodeIds.push_back(varNodeId);
  }
}

void InvariantNode::markDynamicInputTo(VarNodeId varNodeId, bool registerHere) {
  _invariantGraph.varNode(varNodeId).markAsInputFor(_id, false);

  if (registerHere) {
    _dynamicInputVarNodeIds.push_back(varNodeId);
  }
}
}  // namespace atlantis::invariantgraph

#include "invariantgraph/invariantNode.hpp"

#include "invariantgraph/invariantGraph.hpp"

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

bool InvariantNode::prune(InvariantGraph&) { return false; }

void InvariantNode::init(InvariantGraph& invariantGraph,
                         const InvariantNodeId& id) {
  assert(_id == NULL_NODE_ID);
  _id = id;
  for (VarNodeId varNodeId : _outputVarNodeIds) {
    markOutputTo(invariantGraph.varNode(varNodeId), false);
  }
  for (VarNodeId varNodeId : _staticInputVarNodeIds) {
    markStaticInputTo(invariantGraph.varNode(varNodeId), false);
  }
  for (VarNodeId varNodeId : _dynamicInputVarNodeIds) {
    markDynamicInputTo(invariantGraph.varNode(varNodeId), false);
  }
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
  for (size_t i = 0; i < _outputVarNodeIds.size(); ++i) {
    if (_outputVarNodeIds[i] == oldOutputVarNode.varNodeId()) {
      _outputVarNodeIds[i] = newOutputVarNode.varNodeId();
    }
  }
  oldOutputVarNode.unmarkOutputTo(_id);
  newOutputVarNode.markOutputTo(_id);
}

void InvariantNode::removeStaticInputVarNode(VarNode& inputVarNode) {
  // remove all occurrences:
  _staticInputVarNodeIds.erase(
      std::remove(_staticInputVarNodeIds.begin(), _staticInputVarNodeIds.end(),
                  inputVarNode.varNodeId()),
      _staticInputVarNodeIds.end());
  inputVarNode.unmarkAsInputFor(_id, true);
}

void InvariantNode::removeOutputVarNode(VarNode& outputVarNode) {
  // remove all occurrences:
  _outputVarNodeIds.erase(
      std::remove(_outputVarNodeIds.begin(), _outputVarNodeIds.end(),
                  outputVarNode.varNodeId()),
      _outputVarNodeIds.end());
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

propagation::VarId InvariantNode::makeSolverVar(propagation::SolverBase& solver,
                                                VarNode& varNode,
                                                Int initialValue) {
  if (varNode.varId() == propagation::NULL_ID) {
    varNode.setVarId(
        solver.makeIntVar(initialValue, initialValue, initialValue));
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
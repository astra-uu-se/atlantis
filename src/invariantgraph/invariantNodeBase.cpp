#include <cassert>
#include <fznparser/model.hpp>
#include <unordered_map>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a soft constraint (which defines a violation), or a view.
 */
InvariantNodeBase::InvariantNodeBase(std::vector<VarNodeId>&& outputIds,
                                     std::vector<VarNodeId>&& staticInputIds,
                                     std::vector<VarNodeId>&& dynamicInputIds)
    : _outputVarNodeIds(std::move(outputIds)),
      _staticInputVarNodeIds(std::move(staticInputIds)),
      _dynamicInputVarNodeIds(std::move(dynamicInputIds)) {}

void InvariantNodeBase::init(InvariantNodeId id) {
  if (_state == InvariantNodeState::UNINITIALIZED) {
    assert(_id == NULL_NODE_ID);
    _id = id;
    _state = InvariantNodeState::ACTIVE;
  }
}

bool InvariantNodeBase::isReified() const { return false; }

bool InvariantNodeBase::canBeReplaced() const { return false; }

bool InvariantNodeBase::replace() { return false; }

bool InvariantNodeBase::canBeMadeImplicit() const { return false; }

bool InvariantNodeBase::makeImplicit() { return false; }

/**
 * @return The variable nodes defined by this node.
 */
const std::vector<VarNodeId>& InvariantNodeBase::outputVarNodeIds()
    const noexcept {
  return _outputVarNodeIds;
}

/**
 * @return The violation variable of this variable defining node. Only
 * applicable if the current node is a soft constraint. If this node does not
 * define a violation variable, this method returns @p nullptr.
 */
propagation::VarId InvariantNodeBase::violationVarId() const {
  return propagation::NULL_ID;
}

const std::vector<VarNodeId>& InvariantNodeBase::staticInputVarNodeIds()
    const noexcept {
  return _staticInputVarNodeIds;
}

const std::vector<VarNodeId>& InvariantNodeBase::dynamicInputVarNodeIds()
    const noexcept {
  return _dynamicInputVarNodeIds;
}

void InvariantNodeBase::eraseStaticInputVarNode(size_t index) {
  if (index >= _staticInputVarNodeIds.size()) {
    throw InvariantGraphException(
        "InvariantNodeBase::eraseStaticInputVarNode: index out of bounds");
  }
  _staticInputVarNodeIds.erase(_staticInputVarNodeIds.begin() + index);
}

void InvariantNodeBase::eraseDynamicInputVarNode(size_t index) {
  if (index >= _dynamicInputVarNodeIds.size()) {
    throw InvariantGraphException(
        "InvariantNodeBase::eraseDynamicInputVarNode: index out of bounds");
  }
  _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.begin() + index);
}

}  // namespace atlantis::invariantgraph

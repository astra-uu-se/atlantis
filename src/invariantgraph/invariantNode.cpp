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
InvariantNode::InvariantNode(std::shared_ptr<InvariantGraph> invariantGraph,

                             std::vector<VarNodeId>&& outputIds,
                             std::vector<VarNodeId>&& staticInputIds,
                             std::vector<VarNodeId>&& dynamicInputIds)
    : InvariantNodeBase(std::move(outputIds),
                        std::move(staticInputIds) std::move(dynamicInputIds)),
      _invariantGraph(std::move(invariantGraph)) _solver(std::move(solver)) {}

InvariantGraph& InvariantNode::invariantGraph() { return *_invariantGraph; }

std::shared_ptr<InvariantGraph> InvariantNode::invariantGraphPtr() {
  return _invariantGraph;
}

const InvariantGraph& InvariantNode::invariantGraphConst() const {
  return *_invariantGraph;
}

propagation::SolverBase& InvariantNode::solver() { return *_solver; }

const propagation::SolverBase& InvariantNode::solver() const {
  return *_solver;
}

void InvariantNode::init(InvariantNodeId id) {
  InvariantNodeBase.init(id);
  if (_state != InvariantNodeState::ACTIVE) {
    return;
  }
  for (VarNodeId varNodeId : _outputVarNodeIds) {
    markOutputTo(varNodeId, false);
  }
  for (VarNodeId varNodeId : _staticInputVarNodeIds) {
    markStaticInputTo(varNodeId, false);
  }
  for (VarNodeId varNodeId : _dynamicInputVarNodeIds) {
    markDynamicInputTo(varNodeId, false);
  }
}

void InvariantNode::deactivate() {
  while (!staticInputVarNodeIds().empty()) {
    removeStaticInputVarNode(
        _invariantGraph.varNode(staticInputVarNodeIds().front()));
  }
  while (!dynamicInputVarNodeIds().empty()) {
    removeDynamicInputVarNode(
        _invariantGraph.varNode(dynamicInputVarNodeIds().front()));
  }
  while (!outputVarNodeIds().empty()) {
    removeOutputVarNode(_invariantGraph.varNode(outputVarNodeIds().front()));
  }
  _state = InvariantNodeState::SUBSUMED;
}

void InvariantNode::replaceDefinedVar(VarNodeId oldOutputVarNodeId,
                                      VarNodeId newOutputVarNodeId) {
  // Replace all occurrences:
  for (auto& _outputVarNodeId : _outputVarNodeIds) {
    if (_outputVarNodeId == oldOutputVarNodeId.varNodeId()) {
      _outputVarNodeId = newOutputVarNodeId.varNodeId();
    }
  }
  _invariantGraph->varNode(oldOutputVarNodeId).unmarkOutputTo(_id);
  _invariantGraph->varNode(newOutputVarNodeId).markOutputTo(_id);
}

void InvariantNode::removeStaticInputVarNode(VarNodeId retrieveVarNodeId) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_staticInputVarNodeIds.size()) - 1; i >= 0;
       --i) {
    if (_staticInputVarNodeIds[i] == retrieveVarNodeId) {
      _staticInputVarNodeIds.erase(_staticInputVarNodeIds.begin() + i);
    }
  }
  _invariantGraph->varNode(retrieveVarNodeId).unmarkAsInputFor(_id, true);
}

void InvariantNode::removeDynamicInputVarNode(VarNodeId retrieveVarNodeId) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_dynamicInputVarNodeIds.size()) - 1; i >= 0;
       --i) {
    if (_dynamicInputVarNodeIds[i] == retrieveVarNodeId) {
      _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.begin() + i);
    }
  }
  _invariantGraph->varNode(retrieveVarNodeId).unmarkAsInputFor(_id, false);
}

void InvariantNode::removeOutputVarNode(VarNodeId outputVarNodeId) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_outputVarNodeIds.size()) - 1; i >= 0; --i) {
    if (_outputVarNodeIds[i] == outputVarNodeId) {
      _outputVarNodeIds.erase(_outputVarNodeIds.begin() + i);
    }
  }
  _invariantGraph->varNode(outputVarNodeId).unmarkOutputTo(_id);
}

void InvariantNode::replaceStaticInputVarNode(VarNodeId oldInputVarNodeId,
                                              VarNodeId newInputVarNodeId) {
  // Replace all occurrences:
  for (size_t i = 0; i < _staticInputVarNodeIds.size(); ++i) {
    if (_staticInputVarNodeIds[i] == oldInputVarNodeId) {
      _staticInputVarNodeIds[i] = newInputVarNodeId;
    }
  }
  _invariantGraph->varNode(oldInputVarNodeId).unmarkAsInputFor(_id, true);
  _invariantGraph->varNode(newInputVarNodeId).markAsInputFor(_id, true);
}

void InvariantNode::replaceDynamicInputVarNode(VarNodeId oldInputVarNodeId,
                                               VarNodeId newInputVarNodeId) {
  // Replace all occurrences:
  for (size_t i = 0; i < _dynamicInputVarNodeIds.size(); ++i) {
    if (_dynamicInputVarNodeIds[i] == oldInputVarNodeId) {
      _dynamicInputVarNodeIds[i] = newInputVarNodeId;
    }
  }
  _invariantGraph->varNode(oldInputVarNodeId).unmarkAsInputFor(_id, false);
  _invariantGraph->varNode(newInputVarNodeId).markAsInputFor(_id, false);
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

propagation::VarId InvariantNode::makeSolverVar(VarNodeId varNodeId,
                                                Int initialValue) {
  const auto& varNode = _invariantGraph->varNode(varNodeId);
  if (varNode.varId() == propagation::NULL_ID) {
    varNode.setVarId(solver.makeIntVar(
        std::max(varNode.lowerBound(),
                 std::min(varNode.upperBound(), initialValue)),
        varNode.lowerBound(), varNode.upperBound()));
  }
  return varNode.varId();
}

void InvariantNode::markOutputTo(VarNodeId varNodeId, bool registerHere) {
  _invariantGraph->varNode(varNodeId).markOutputTo(_id);

  if (registerHere) {
    _outputVarNodeIds.push_back(varNodeId);
  }
}

void InvariantNode::markStaticInputTo(VarNodeId varNodeId, bool registerHere) {
  _invariantGraph->varNode(varNodeId).markAsInputFor(_id, true);

  if (registerHere) {
    _staticInputVarNodeIds.push_back(varNodeId);
  }
}

void InvariantNode::markDynamicInputTo(VarNodeId varNodeId, bool registerHere) {
  _invariantGraph->varNode(varNodeId).markAsInputFor(_id, false);

  if (registerHere) {
    _dynamicInputVarNodeIds.push_back(varNodeId);
  }
}
}  // namespace atlantis::invariantgraph

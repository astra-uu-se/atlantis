#include "propagation/propagation/propagationGraph.hpp"

namespace atlantis::propagation {

inline bool all_in_range(size_t start, size_t stop,
                         std::function<bool(size_t)> predicate) {
  std::vector<size_t> vec(stop - start);
  for (size_t i = 0; i < stop - start; ++i) {
    vec.at(i) = start + i;
  }
  return std::all_of(vec.begin(), vec.end(), predicate);
}

PropagationGraph::PropagationGraph(const Store& store, size_t expectedSize)
    : _store(store),
      _definingInvariant(expectedSize),
      _varsDefinedByInvariant(expectedSize),
      _inputVars(expectedSize),
      _isDynamicInvariant(expectedSize),
      _listeningInvariantData(expectedSize),
      _varLayerIndex(expectedSize),
      _varPosition(expectedSize) {}

void PropagationGraph::registerInvariant(InvariantId invariantId) {
  // Everything must be registered in sequence.
  _varsDefinedByInvariant.register_idx(invariantId);
  _isDynamicInvariant.register_idx(invariantId, false);
  _inputVars.register_idx(invariantId);
  ++_numInvariants;
}

void PropagationGraph::registerVar(VarIdBase id) {
  _definingInvariant.register_idx(id);
  _listeningInvariantData.register_idx(id);
  _varLayerIndex.register_idx(id);
  _varPosition.register_idx(id);
  ++_numVars;
}

void PropagationGraph::registerInvariantInput(InvariantId invariantId,
                                              VarIdBase varId, LocalId localId,
                                              bool isDynamicInput) {
  assert(!invariantId.equals(NULL_ID) && !varId.equals(NULL_ID));
  if (_definingInvariant[varId] == invariantId) {
    logWarning("The invariant (" << invariantId << ") already "
                                 << "defines the varId variable (" << varId
                                 << "); "
                                 << "ignoring (selft-cyclic) dependency.");
    return;
  }
  _isDynamicInvariant.set(
      invariantId, _isDynamicInvariant.get(invariantId) || isDynamicInput);
  _listeningInvariantData[varId].push_back(
      ListeningInvariantData(invariantId, localId));
  _inputVars[invariantId].push_back(
      std::pair<VarIdBase, bool>{varId, isDynamicInput});
}

void PropagationGraph::registerDefinedVar(VarIdBase varId,
                                          InvariantId invariantId) {
  assert(!varId.equals(NULL_ID) && !invariantId.equals(NULL_ID));
  if (_definingInvariant.at(varId).id != NULL_ID.id) {
    throw VarAlreadyDefinedException(
        "Variable " + std::to_string(varId.id) +
        " already defined by invariant " +
        std::to_string(_definingInvariant.at(varId).id));
  }
  size_t index = _listeningInvariantData[varId].size();
  for (size_t i = 0; i < _listeningInvariantData[varId].size(); ++i) {
    if (_listeningInvariantData[varId][i].invariantId == invariantId) {
      index = i;
      break;
    }
  }
  if (index < _listeningInvariantData[varId].size()) {
    _listeningInvariantData[varId].erase(
        _listeningInvariantData[varId].begin() + index);
    logWarning("The (self-cyclic) dependency that the invariant "
               << "(" << invariantId << ") depends on the input "
               << "variable (" << invariantId << ") was removed.");
    assert(std::all_of(_listeningInvariantData[varId].begin(),
                       _listeningInvariantData[varId].end(),
                       [&](const ListeningInvariantData& data) {
                         return data.invariantId != invariantId;
                       }));
  }
  _definingInvariant[varId] = invariantId;
  _varsDefinedByInvariant[invariantId].push_back(varId);
}

void PropagationGraph::close(Timestamp ts) {
  _isSearchVar.resize(numVars() + 1);
  _isEvaluationVar.resize(numVars() + 1);
  _evaluationVars.clear();
  _searchVars.clear();
  for (size_t i = 1; i < numVars() + 1; ++i) {
    _isEvaluationVar[i] = (_listeningInvariantData.at(i).empty());
    _isSearchVar[i] = (_definingInvariant.at(i) == NULL_ID);
    if (_isEvaluationVar[i]) {
      _evaluationVars.emplace_back(i);
    }
    if (_isSearchVar[i]) {
      _searchVars.emplace_back(i);
    }
  }

  partitionIntoLayers();
  mergeLayersWithoutDynamicCycles();
  computeLayerOffsets();
  topologicallyOrder(ts);
  // Reset propagation queue data structure.
  // TODO: Be sure that this does not cause a memeory leak...
  // _propagationQueue = PropagationQueue();
  _propagationQueue.init(numVars(), numLayers());
  for (size_t i = 1; i < numVars() + 1; ++i) {
    _propagationQueue.initVar(VarIdBase(i), position(VarIdBase(i)));
  }
}

bool PropagationGraph::containsStaticCycle(std::vector<bool>& visited,
                                           std::vector<bool>& inFrontier,
                                           VarIdBase varId) {
  // Mark current output variable
  assert(varId < visited.size());
  assert(varId < inFrontier.size());
  if (inFrontier[varId]) {
    return true;
  }
  visited[varId] = true;
  inFrontier[varId] = true;
  // get the defining invariant:
  const InvariantId defInv = definingInvariant(varId);
  if (defInv != NULL_ID) {
    for (const auto& [inputId, isDynamicInput] : inputVars(defInv)) {
      if (!isDynamicInput &&
          containsStaticCycle(visited, inFrontier, inputId)) {
        return true;
      }
    }
  }
  inFrontier[varId] = false;
  return false;
}

bool PropagationGraph::containsStaticCycle() {
  std::vector<bool> visited(numVars() + 1, false);
  std::vector<bool> inFrontier(numVars() + 1, false);
  // Check for static cycles starting from the output variables:
  for (size_t varId = 1u; varId <= numVars(); ++varId) {
    assert(all_in_range(1u, numVars() + 1,
                        [&](const size_t i) { return !inFrontier.at(i); }));
    if (listeningInvariantData(varId).size() == 0) {
      if (containsStaticCycle(visited, inFrontier, VarIdBase(varId))) {
        return true;
      }
    }
  }
  assert(all_in_range(1u, numVars() + 1,
                      [&](const size_t varId) { return visited.at(varId); }));
  return false;
}

void PropagationGraph::partitionIntoLayers(std::vector<bool>& visited,
                                           VarIdBase varId) {
  assert(_varLayerIndex.has_idx(varId));
  assert(varId < visited.size());
  // Mark current output variable
  visited[varId] = true;
  // get the defining invariant:
  const InvariantId defInv = definingInvariant(varId);
  size_t layer = 0;
  if (defInv != NULL_ID) {
    // we are at a defined variable.
    // for each input variable:
    for (const auto& [inputId, isDynamicInput] : inputVars(defInv)) {
      // visit the input if unvisited:
      if (!visited[inputId]) {
        partitionIntoLayers(visited, inputId);
      }
      // update layer for varId:
      layer =
          std::max(layer, _varLayerIndex[inputId].layer +
                              static_cast<size_t>(isDynamicInvariant(defInv) &&
                                                  !isDynamicInput));
    }
  }
  _varLayerIndex[varId].layer = layer;
  for (size_t i = _varsInLayer.size(); i <= layer; ++i) {
    _varsInLayer.emplace_back(std::vector<VarIdBase>{});
  }
  _varLayerIndex[varId].index = _varsInLayer[layer].size();
  _varsInLayer[layer].emplace_back(varId);
}

/**
 * Computes a topological sort from a dependency graph with cycles by
 * non-deterministically ignoring one edge in each cycle.
 * This means that there will be an order within cycles.
 *
 * Gives different key-domains to Variables and invariants.
 * That is, the key of invariants cannot be compared with variables.
 *
 * Variables that are in the same propagation layer will (most of the time)
 * share key-value.
 */
void PropagationGraph::partitionIntoLayers() {
  std::vector<bool> visited(numVars() + 1, false);
  _layerHasDynamicCycle.assign(1, false);
  _varsInLayer.assign(1, std::vector<VarIdBase>{});
  assert(_varLayerIndex.size() == numVars());
  // Call visit on all output variables
  for (const VarIdBase evalVar : _evaluationVars) {
    partitionIntoLayers(visited, evalVar);
  }
  // Visit any unvisited nodes (this should not happen):
  for (size_t varId = 1u; varId <= numVars(); ++varId) {
    if (!visited[varId]) {
      partitionIntoLayers(visited, VarIdBase(varId));
    }
  }
  assert(all_in_range(1u, numVars() + 1, [&](const size_t varId) {
    const size_t layer = _varLayerIndex.at(varId).layer;
    const size_t index = _varLayerIndex.at(varId).index;
    return _varLayerIndex.has_idx(varId) && layer < _varsInLayer.size() &&
           index < _varsInLayer.at(layer).size() &&
           varId == _varsInLayer.at(layer).at(index).id;
  }));
}

bool PropagationGraph::containsDynamicCycle(std::vector<bool>& visited,
                                            VarIdBase originVarId) {
  assert(_varLayerIndex.has_idx(originVarId));
  const size_t layer = _varLayerIndex[originVarId].layer;
  assert(layer < numLayers());

  std::vector<bool> onStack(visited.size(), false);
  std::vector<VarIdBase> stack;
  stack.reserve(visited.size());
  stack.emplace_back(originVarId);
  size_t stackPtr = 0;

  while (stackPtr < stack.size()) {
    const VarIdBase varId = stack[stackPtr];
    ++stackPtr;
    assert(varId != NULL_ID);
    const size_t index = _varLayerIndex[varId].index;
    assert(index < _varsInLayer.at(layer).size());
    assert(varId == _varsInLayer.at(layer).at(index));
    assert(index < visited.size());
    assert(index < onStack.size());

    if (visited[index]) {
      // this node has been visited during a previous origin var
      continue;
    }
    if (onStack[index]) {
      // this node has been pushed onto the stack, there is a cycle
      return true;
    }
    onStack[index] = true;
    const InvariantId defInv = definingInvariant(varId);
    if (defInv == NULL_ID) {
      // we are at a search variable.
      return false;
    }
    // mark as in frontier:
    onStack[index] = true;
    // get the defining invariant:
    for (const auto& [inputId, _] : inputVars(defInv)) {
      assert(_varLayerIndex[inputId].layer <= layer);
      if (_varLayerIndex[inputId].layer == layer) {
        stack.emplace_back(inputId);
      }
    }
  }
  for (const VarIdBase varId : stack) {
    // add all nodes that have been visited during this call to visited
    visited[_varLayerIndex[varId].index] = true;
  }
  return false;
}

bool PropagationGraph::containsDynamicCycle(size_t layer) {
  assert(layer < numLayers());
  std::vector<bool> visited(_varsInLayer[layer].size(), false);
  // Check for dynamic cycles starting from the output variables:
  for (const VarIdBase varId : _varsInLayer[layer]) {
    if (!visited[varId] && definingInvariant(varId) != NULL_ID &&
        isDynamicInvariant(definingInvariant(varId)) &&
        containsDynamicCycle(visited, varId)) {
      return true;
    }
  }
  return false;
}

void PropagationGraph::mergeLayersWithoutDynamicCycles() {
  // combine subsequent layers without dynamic cycles:
  assert(!containsDynamicCycle(0));
  _layerHasDynamicCycle.assign(numLayers(), false);
  size_t layer = 1;
  while (layer < numLayers()) {
    _layerHasDynamicCycle[layer] = containsDynamicCycle(layer);
    if (_layerHasDynamicCycle[layer - 1] || _layerHasDynamicCycle[layer]) {
      // the previous layer had one or more dynamic cycles: continue
      ++layer;
      continue;
    }
    // The previous layer contained no cycles: merge the layers.
    // merge layers layer - 1 and layer:
    const size_t oldSize = _varsInLayer[layer - 1].size();
    // concat layer to layer - 1:
    _varsInLayer[layer - 1].insert(_varsInLayer[layer - 1].end(),
                                   _varsInLayer[layer].begin(),
                                   _varsInLayer[layer].end());
    // Update the vars:
    for (size_t i = oldSize; i < _varsInLayer[layer - 1].size(); ++i) {
      const VarIdBase varId = _varsInLayer[layer - 1][i];
      // sanity:
      assert(_varLayerIndex.at(varId).layer == layer);
      assert(_varsInLayer[layer][_varLayerIndex.at(varId).index] == varId);
      // change layer
      _varLayerIndex[varId].layer = layer - 1;
      _varLayerIndex[varId].index = i;
      // sanity
      assert(varId == _varsInLayer[_varLayerIndex[varId].layer]
                                  [_varLayerIndex[varId].index]);
    }
    assert(std::all_of(_varsInLayer[layer - 1].begin(),
                       _varsInLayer[layer - 1].end(),
                       [&](const VarIdBase varId) {
                         return _varLayerIndex[varId].layer == layer - 1;
                       }));

    assert(!containsDynamicCycle(layer - 1));
    // shift the rest of the layers:
    for (size_t l = layer + 1; l < numLayers(); ++l) {
      for (const VarIdBase varId : _varsInLayer[l]) {
        _varLayerIndex[varId].layer = l - 1;
      }
    }
    // remove the layer
    _varsInLayer.erase(_varsInLayer.begin() + layer);
    _layerHasDynamicCycle.erase(_layerHasDynamicCycle.begin() + layer);
  }
}

void PropagationGraph::computeLayerOffsets() {
  _layerPositionOffset.assign(numLayers(), 0);
  for (size_t layer = 1; layer < numLayers(); ++layer) {
    _layerPositionOffset[layer] =
        _layerPositionOffset[layer - 1] + _varsInLayer[layer - 1].size();
  }
  assert(_layerPositionOffset.back() + _varsInLayer.back().size() == numVars());
}

void PropagationGraph::topologicallyOrder(const Timestamp ts,
                                          std::vector<bool>& inFrontier,
                                          const VarIdBase varId) {
  assert(_varLayerIndex.has_idx(varId));
  assert(_varLayerIndex.at(varId).layer < numLayers());
  const auto& [layer, index] = _varLayerIndex.at(varId);

  // sanity:
  assert(_varLayerIndex.at(varId).index < _varsInLayer[layer].size());
  assert(layer < _varsInLayer.size());
  assert(index < _varsInLayer.at(layer).size());
  assert(index < inFrontier.size());
  assert(varId == _varsInLayer.at(layer).at(index));

  if (inFrontier[index]) {
    throw TopologicalOrderError();
  }
  assert(_varPosition[varId] == numVars());
  if (_varPosition[varId] != numVars()) {
    // already visited:
    return;
  }

  // Get defining invariant:
  const InvariantId defInv = definingInvariant(varId);

  // reset the topological number:
  _varPosition[varId] = 0;

  if (defInv == NULL_ID) {
    // The current variable is a search variable:
    return;
  }

  // add the current variable to the frontier:
  inFrontier[index] = true;

  const bool isDynInv =
      _layerHasDynamicCycle[layer] && isDynamicInvariant(defInv);

  assert(std::all_of(inputVars(defInv).begin(), inputVars(defInv).end(),
                     [&](const std::pair<VarId, bool>& p) {
                       if (p.first == NULL_ID) {
                         return false;
                       }
                       if (isDynInv && !p.second) {
                         return _varLayerIndex[p.first].layer < layer;
                       }
                       return _varLayerIndex[p.first].layer <= layer;
                     }));

  for (const auto& [inputId, isDynamicInput] : _inputVars[defInv]) {
    if (!isDynInv || !isDynamicInput) {
      if (_varLayerIndex[inputId].layer == layer &&
          _varPosition[inputId] == numVars()) {
        topologicallyOrder(ts, inFrontier, inputId);
      }

      _varPosition[varId] =
          std::max(_varPosition[varId], _varPosition[inputId] + 1);
    }
  }
  if (isDynInv) {
    const VarId dynamicInputId = dynamicInputVar(ts, defInv);
    assert(dynamicInputId != NULL_ID);
    // we should have no dependencies to subsequent layers:
    assert(_varLayerIndex[dynamicInputId].layer <= layer);
    if (_varLayerIndex[dynamicInputId].layer == layer &&
        _varPosition[dynamicInputId] == numVars()) {
      topologicallyOrder(ts, inFrontier, dynamicInputId);
    }
    _varPosition[varId] =
        std::max(_varPosition[varId], _varPosition[dynamicInputId] + 1);
  }
  assert(std::all_of(inputVars(defInv).begin(), inputVars(defInv).end(),
                     [&](const std::pair<VarId, bool> p) {
                       if (p.first == NULL_ID) {
                         return false;
                       }
                       if (isDynInv && p.second) {
                         return _store.dynamicInputVar(ts, defInv) != p.first ||
                                _varPosition[p.first] < _varPosition[varId];
                       }
                       return _varPosition[p.first] < _varPosition[varId];
                     }));

  inFrontier[index] = false;
}

/**
 * Computes a topological sort from a dependency graph with cycles by
 * non-deterministically ignoring one edge in each cycle.
 * This means that there will be an order within cycles.
 *
 * Gives different key-domains to Variables and invariants.
 * That is, the key of invariants cannot be compared with variables.
 *
 * Variables that are in the same propagation layer will (most of the time)
 * share key-value.
 */
void PropagationGraph::topologicallyOrder(Timestamp ts, size_t layer,
                                          bool updatePriorityQueue) {
  assert(layer < numLayers());
  assert(_layerPositionOffset.size() == numLayers());
  for (const VarIdBase varId : _varsInLayer[layer]) {
    _varPosition[varId] = numVars();
  }
  std::vector<bool> inFrontier(_varsInLayer[layer].size(), false);
  for (const VarIdBase varId : _varsInLayer[layer]) {
    if (_varPosition[varId] == numVars()) {
      topologicallyOrder(ts, inFrontier, varId);
    }
    assert(_varPosition[varId] < numVars());
  }
  assert(all_in_range(0u, inFrontier.size(), [&](const size_t index) {
    return !inFrontier.at(index);
  }));
#ifndef NDEBUG
  for (const VarIdBase varId : _varsInLayer.at(layer)) {
    const InvariantId defInv = definingInvariant(varId);
    if (defInv == NULL_ID) {
      assert(_varPosition[varId] == 0);
      continue;
    }
    const bool isDynInv =
        _layerHasDynamicCycle.at(layer) && isDynamicInvariant(defInv);
    for (const auto& [inputId, isDynamicInput] : inputVars(defInv)) {
      if (!isDynInv) {
        assert(_varPosition[inputId] < _varPosition[varId]);
      } else if (!isDynamicInput) {
        assert(_varPosition[inputId] < _varPosition[varId]);
      } else if (dynamicInputVar(ts, defInv) == inputId) {
        assert(_varPosition[inputId] < _varPosition[varId]);
      }
    }
  }
#endif

  if (updatePriorityQueue) {
    for (const VarIdBase varId : _varsInLayer[layer]) {
      _propagationQueue.updatePriority(varId, _varPosition.at(varId));
    }
  }
}

void PropagationGraph::topologicallyOrder(Timestamp ts) {
  for (size_t layer = 0; layer < numLayers(); ++layer) {
    topologicallyOrder(ts, layer, false);
  }
}
}  // namespace atlantis::propagation
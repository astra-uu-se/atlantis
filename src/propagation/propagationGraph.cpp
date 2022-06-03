#include "propagation/propagationGraph.hpp"

#include "misc/logging.hpp"

PropagationGraph::PropagationGraph(const Store& store, size_t expectedSize)
    : _store(store),
      _definingInvariant(expectedSize),
      _variablesDefinedByInvariant(expectedSize),
      _inputVariables(expectedSize),
      _isDynamicInvariant(expectedSize),
      _listeningInvariants(expectedSize),
      _variableLayerIndex(expectedSize),
      _variablePosition(expectedSize) {}

void PropagationGraph::registerInvariant(InvariantId invariantId) {
  // Everything must be registered in sequence.
  _variablesDefinedByInvariant.register_idx(invariantId);
  _isDynamicInvariant.register_idx(invariantId, false);
  _inputVariables.register_idx(invariantId);
  ++_numInvariants;
}

void PropagationGraph::registerVar(VarIdBase id) {
  _definingInvariant.register_idx(id);
  _listeningInvariants.register_idx(id);
  _variableLayerIndex.register_idx(id);
  _variablePosition.register_idx(id);
  ++_numVariables;
}

void PropagationGraph::registerInvariantInput(InvariantId invariantId,
                                              VarIdBase varId,
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
  _listeningInvariants[varId].push_back(invariantId);
  _inputVariables[invariantId].push_back(
      std::pair<VarIdBase, bool>{varId, isDynamicInput});
}

void PropagationGraph::registerDefinedVariable(VarIdBase varId,
                                               InvariantId invariantId) {
  assert(!varId.equals(NULL_ID) && !invariantId.equals(NULL_ID));
  if (_definingInvariant.at(varId).id != NULL_ID.id) {
    throw VariableAlreadyDefinedException(
        "Variable " + std::to_string(varId.id) +
        " already defined by invariant " +
        std::to_string(_definingInvariant.at(varId).id));
  }
  size_t index = _listeningInvariants[varId].size();
  for (size_t i = 0; i < _listeningInvariants[varId].size(); ++i) {
    if (_listeningInvariants[varId][i] == invariantId) {
      index = i;
      break;
    }
  }
  if (index < _listeningInvariants[varId].size()) {
    _listeningInvariants[varId].erase(_listeningInvariants[varId].begin() +
                                      index);
    logWarning("The (self-cyclic) dependency that the invariant "
               << "(" << invariantId << ") depends on the input "
               << "variable (" << invariantId << ") was removed.");
  }
  _definingInvariant[varId] = invariantId;
  _variablesDefinedByInvariant[invariantId].push_back(varId);
}

void PropagationGraph::close(Timestamp ts) {
  _isSearchVariable.resize(numVariables() + 1);
  _isEvaluationVariable.resize(numVariables() + 1);
  _evaluationVariables.clear();
  _searchVariables.clear();
  for (size_t i = 1; i < numVariables() + 1; ++i) {
    _isEvaluationVariable[i] = (_listeningInvariants.at(i).empty());
    _isSearchVariable[i] = (_definingInvariant.at(i) == NULL_ID);
    if (_isEvaluationVariable[i]) {
      _evaluationVariables.emplace_back(i);
    }
    if (_isSearchVariable[i]) {
      _searchVariables.emplace_back(i);
    }
  }

  partitionIntoLayers();
  mergeLayersWithoutDynamicCycles();
  computeLayerOffsets();
  topologicallyOrder(ts);
  // Reset propagation queue data structure.
  // TODO: Be sure that this does not cause a memeory leak...
  // _propagationQueue = PropagationQueue();
  _propagationQueue.init(numVariables(), numLayers());
  for (size_t i = 1; i < numVariables() + 1; ++i) {
    _propagationQueue.initVar(VarIdBase(i), position(VarIdBase(i)));
  }
}

bool PropagationGraph::containsStaticCycle(std::vector<bool>& visited,
                                           std::vector<bool>& inPenumbra,
                                           VarIdBase varId) {
  // Mark current output variable
  assert(varId < visited.size());
  assert(varId < inPenumbra.size());
  if (inPenumbra[varId]) {
    return true;
  }
  visited[varId] = true;
  inPenumbra[varId] = true;
  // get the defining invariant:
  const InvariantId defInv = definingInvariant(varId);
  if (defInv != NULL_ID) {
    for (const auto& [inputId, isDynamicInput] : inputVariables(defInv)) {
      if (!isDynamicInput &&
          containsStaticCycle(visited, inPenumbra, inputId)) {
        return true;
      }
    }
  }
  inPenumbra[varId] = false;
  return false;
}

bool PropagationGraph::containsStaticCycle() {
  std::vector<bool> visited(numVariables() + 1, false);
  std::vector<bool> inPenumbra(numVariables() + 1, false);
  // Check for static cycles starting from the output variables:
  for (size_t varId = 1u; varId <= numVariables(); ++varId) {
#ifndef NDEBUG
    for (size_t i = 1u; i <= numVariables(); ++i) {
      assert(!inPenumbra[i]);
    }
#endif
    if (listeningInvariants(varId).size() == 0) {
      if (containsStaticCycle(visited, inPenumbra, VarIdBase(varId))) {
        return true;
      }
    }
  }
#ifndef NDEBUG
  for (size_t varId = 1u; varId <= numVariables(); ++varId) {
    assert(visited[varId]);
  }
#endif
  return false;
}

void PropagationGraph::partitionIntoLayers(std::vector<bool>& visited,
                                           VarIdBase varId) {
  assert(_variableLayerIndex.has_idx(varId));
  assert(varId < visited.size());
  // Mark current output variable
  visited[varId] = true;
  // get the defining invariant:
  const InvariantId defInv = definingInvariant(varId);
  size_t layer = 0;
  if (defInv != NULL_ID) {
    // we are at a defined variable.
    // for each input variable:
    for (const auto& [inputId, isDynamicInput] : inputVariables(defInv)) {
      // visit the input if unvisited:
      if (!visited[inputId]) {
        partitionIntoLayers(visited, inputId);
      }
      // update layer for varId:
      layer =
          std::max(layer, _variableLayerIndex[inputId].layer +
                              static_cast<size_t>(isDynamicInvariant(defInv) &&
                                                  !isDynamicInput));
    }
  }
  _variableLayerIndex[varId].layer = layer;
  for (size_t i = _variablesInLayer.size(); i <= layer; ++i) {
    _variablesInLayer.emplace_back(std::vector<VarIdBase>{});
  }
  _variableLayerIndex[varId].index = _variablesInLayer[layer].size();
  _variablesInLayer[layer].emplace_back(varId);
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
  std::vector<bool> visited(numVariables() + 1, false);
  _layerHasDynamicCycle.assign(1, false);
  _variablesInLayer.assign(1, std::vector<VarIdBase>{});
  assert(_variableLayerIndex.size() == numVariables());
  // Call visit on all output variables
  for (const VarIdBase evalVar : _evaluationVariables) {
    partitionIntoLayers(visited, evalVar);
  }
  // Visit any unvisited nodes (this should not happen):
  for (size_t varId = 1u; varId <= numVariables(); ++varId) {
    if (!visited[varId]) {
      partitionIntoLayers(visited, VarIdBase(varId));
    }
  }
#ifndef NDEBUG
  for (size_t varId = 1; varId <= numVariables(); ++varId) {
    assert(_variableLayerIndex.has_idx(varId));
    const size_t layer = _variableLayerIndex.at(varId).layer;
    const size_t index = _variableLayerIndex.at(varId).index;
    assert(layer < _variablesInLayer.size());
    assert(index < _variablesInLayer.at(layer).size());
    assert(varId == _variablesInLayer.at(layer).at(index).id);
  }
#endif
}

bool PropagationGraph::containsDynamicCycle(std::vector<bool>& visited,
                                            std::vector<bool>& inPenumbra,
                                            VarIdBase varId) {
  assert(_variableLayerIndex.has_idx(varId));
  // Mark current invariant as visited:
  const size_t layer = _variableLayerIndex[varId].layer;
  assert(layer < numLayers());
  const size_t index = _variableLayerIndex[varId].index;
  assert(index < _variablesInLayer.at(layer).size());
  assert(varId == _variablesInLayer.at(layer).at(index));
  assert(index < visited.size());
  assert(index < inPenumbra.size());

  // mark as visited:
  visited[index] = true;

  if (inPenumbra[index]) {
    // the variable is already in the penumbra: there is a cycle.
    return true;
  }
  const InvariantId defInv = definingInvariant(varId);
  if (defInv == NULL_ID) {
    // we are at a search variable.
    return false;
  }
  // mark as in penumbra:
  inPenumbra[index] = true;
  // get the defining invariant:
  for (const auto& [inputId, _] : inputVariables(defInv)) {
    assert(_variableLayerIndex[inputId].layer <= layer);
    if (_variableLayerIndex[inputId].layer == layer) {
      if (containsDynamicCycle(visited, inPenumbra, inputId)) {
        return true;
      }
    }
  }
  inPenumbra[index] = false;
  return false;
}

bool PropagationGraph::containsDynamicCycle(size_t layer) {
  assert(layer < numLayers());
  std::vector<bool> visited(_variablesInLayer[layer].size(), false);
  std::vector<bool> inPenumbra(_variablesInLayer[layer].size(), false);
  // Check for static cycles starting from the output variables:
  for (size_t i = 0; i < _variablesInLayer[layer].size(); ++i) {
#ifndef NDEBUG
    for (size_t varId = 0; varId <= _variablesInLayer[layer].size(); ++varId) {
      assert(!inPenumbra[varId]);
    }
#endif
    if (!visited[i]) {
      if (containsDynamicCycle(visited, inPenumbra,
                               _variablesInLayer[layer][i])) {
        return true;
      }
    }
  }
#ifndef NDEBUG
  for (size_t i = 0; i < _variablesInLayer[layer].size(); ++i) {
    assert(visited[i]);
  }
#endif
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
    for (const VarIdBase varId : _variablesInLayer[layer]) {
      // sanity:
      assert(_variableLayerIndex.at(varId).layer == layer);
      assert(_variablesInLayer[layer][_variableLayerIndex.at(varId).index] ==
             varId);
      // change layer
      _variableLayerIndex[varId].layer = layer - 1;
      _variableLayerIndex[varId].index = _variablesInLayer[layer - 1].size();
      _variablesInLayer[layer - 1].emplace_back(varId);
      // sanity
      assert(varId == _variablesInLayer[_variableLayerIndex[varId].layer]
                                       [_variableLayerIndex[varId].index]);
    }
    assert(!containsDynamicCycle(layer - 1));
    // shift the rest of the layers:
    for (size_t l = layer + 1; l < numLayers(); ++l) {
      for (const VarIdBase varId : _variablesInLayer[l]) {
        _variableLayerIndex[varId].layer = l - 1;
      }
    }
    // remove the layer
    _variablesInLayer.erase(_variablesInLayer.begin() + layer);
    _layerHasDynamicCycle.erase(_layerHasDynamicCycle.begin() + layer);
  }
}

void PropagationGraph::computeLayerOffsets() {
  _layerPositionOffset.assign(numLayers(), 0);
  for (size_t layer = 1; layer < numLayers(); ++layer) {
    _layerPositionOffset[layer] =
        _layerPositionOffset[layer - 1] + _variablesInLayer[layer - 1].size();
  }
  assert(_layerPositionOffset.back() + _variablesInLayer.back().size() ==
         numVariables());
}

size_t PropagationGraph::topologicallyOrder(Timestamp ts, VarIdBase varId,
                                            size_t curPosition) {
  assert(_variableLayerIndex.has_idx(varId));
  assert(_variableLayerIndex.at(varId).layer < numLayers());
  const size_t layer = _variableLayerIndex.at(varId).layer;
  assert(_variableLayerIndex.at(varId).index < _variablesInLayer[layer].size());
  const size_t index = _variableLayerIndex.at(varId).index;
  // sanity:
  assert(layer < _variablesInLayer.size());
  assert(index < _variablesInLayer.at(layer).size());
  assert(varId == _variablesInLayer.at(layer).at(index));
  assert(curPosition < numVariables());
  assert(layer + 1 >= numLayers() ||
         curPosition < _layerPositionOffset[layer + 1]);

  if (_variablePosition[varId] != numVariables()) {
    // already visited:
    return curPosition;
  }
  // Mark invariant as visited
  const InvariantId defInv = definingInvariant(varId);
  if (defInv == NULL_ID) {
    // This variable is a search variable:
    _variablePosition[varId] = curPosition;
    return curPosition + 1;
  }
  if (_layerHasDynamicCycle[layer] && isDynamicInvariant(defInv)) {
    // Layer with dynamic cycle and defInv is dynamic:
    const VarId dynamicInputId = dynamicInputVariable(ts, defInv);
    // we should have no dependencies to subsequent layers:
    assert(dynamicInputId == NULL_ID ||
           _variableLayerIndex[dynamicInputId].layer <= layer);
    if (dynamicInputId != NULL_ID &&
        _variableLayerIndex[dynamicInputId].layer == layer) {
      // visit dynamic input, retrieving the updated position:
      curPosition = topologicallyOrder(ts, dynamicInputId, curPosition);
    }
    _variablePosition[varId] = curPosition;
    return curPosition + 1;
  }
  // The layer has no cycles or defInv is static:
  for (const auto& [inputId, isDynamicInput] : inputVariables(defInv)) {
    // sanity check:
    assert(!_layerHasDynamicCycle.at(layer) || !isDynamicInput);
    assert(inputId != NULL_ID);
    assert(_variableLayerIndex[inputId].layer <= layer);
    if (_variableLayerIndex[inputId].layer == layer) {
      // visit defining invariant, retrieving the updated position:
      curPosition = topologicallyOrder(ts, inputId, curPosition);
    }
  }
  _variablePosition[varId] = curPosition;
  return curPosition + 1;
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
  for (const VarIdBase varId : _variablesInLayer[layer]) {
    _variablePosition[varId] = numVariables();
  }

  for (const VarIdBase varId : _variablesInLayer[layer]) {
    if (_variablePosition[varId] == numVariables()) {
      topologicallyOrder(ts, varId, _layerPositionOffset[layer]);
    }
  }
  if (updatePriorityQueue) {
    for (const VarIdBase varId : _variablesInLayer[layer]) {
      _propagationQueue.updatePriority(varId, _variablePosition.at(varId));
    }
  }
}

void PropagationGraph::topologicallyOrder(Timestamp ts) {
  for (size_t layer = 0; layer < numLayers(); ++layer) {
    topologicallyOrder(ts, layer, false);
  }
}
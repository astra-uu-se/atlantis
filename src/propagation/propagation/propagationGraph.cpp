#include "atlantis/propagation/propagation/propagationGraph.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <numeric>
#include <ranges>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/store/store.hpp"

namespace atlantis::propagation {

inline bool all_in_range(size_t start, size_t stop,
                         std::function<bool(size_t)>&& predicate) {
  std::vector<size_t> vec(stop - start);
  for (size_t i = 0; i < stop - start; ++i) {
    vec.at(i) = start + i;
  }
  return std::ranges::all_of(vec.begin(), vec.end(), std::move(predicate));
}

static void SCCUtil(const PropagationGraph& graph, VarId inputId,
                    std::vector<Int>& discoverTime, std::vector<Int>& lowTime,
                    std::vector<VarId>& stack, std::vector<bool>& onStack,
                    Int& time, std::vector<std::vector<VarId>>& components) {
  assert(inputId < discoverTime.size());
  assert(discoverTime.size() == lowTime.size());
  assert(discoverTime.size() == onStack.size());
  assert(!onStack[inputId]);
  discoverTime[inputId] = lowTime[inputId] = time;
  ++time;
  stack.emplace_back(inputId);
  onStack[inputId] = true;

  for (const auto& data : graph.listeningInvariantData(inputId)) {
    for (const VarId outputId : graph.varsDefinedBy(data.invariantId)) {
      if (discoverTime[outputId] < 0) {
        SCCUtil(graph, outputId, discoverTime, lowTime, stack, onStack, time,
                components);
        lowTime[inputId] = std::min(lowTime[outputId], lowTime[inputId]);
      } else if (onStack[outputId]) {
        lowTime[inputId] = std::min(lowTime[outputId], discoverTime[inputId]);
      }
    }
  }
  if (lowTime[inputId] == discoverTime[inputId]) {
    const bool inSCC = stack.back() != inputId;
    if (inSCC) {
      components.emplace_back();
      while (stack.back() != inputId) {
        onStack[stack.back()] = false;
        components.back().emplace_back(stack.back());
        stack.pop_back();
      }
    }
    onStack[inputId] = false;
    assert(stack.back() == inputId);
    if (inSCC) {
      components.back().emplace_back(inputId);
    }
    stack.pop_back();
  }
}

static std::vector<std::vector<VarId>> SCC(const PropagationGraph& graph) {
  std::vector<Int> discoverTime(graph.numVars(), -1);
  std::vector<Int> lowTime(graph.numVars(), -1);
  std::vector<VarId> stack;
  stack.reserve(graph.numVars());
  std::vector<bool> onStack(graph.numVars(), false);
  std::vector<std::vector<VarId>> components;
  components.reserve(graph.numVars());
  Int time = 0;
  for (const VarId searchVar : graph.searchVars()) {
    if (discoverTime[searchVar] < 0) {
      SCCUtil(graph, searchVar, discoverTime, lowTime, stack, onStack, time,
              components);
    }
  }
  assert(std::ranges::all_of(discoverTime, [&](Int i) { return i >= 0; }));
  return components;
}

static void partitionIntoLayersUtil(
    const PropagationGraph& graph,
    const std::vector<std::vector<VarId>>& components, VarId varId,
    const std::vector<size_t>& componentOfVar, std::vector<bool>& visited,
    std::vector<VarId>& layerOfVar, std::vector<bool>& layerHasSCC) {
  visited[varId] = true;
  const InvariantId defInv = graph.definingInvariant(varId);
  if (defInv == NULL_ID) {
    // varId is a search variable, put into layer 0:
    assert(componentOfVar[varId] >= components.size());
    layerOfVar[varId] = 0;
    if (layerHasSCC.empty()) {
      layerHasSCC.emplace_back(false);
    }
    assert(!layerHasSCC[0]);
    return;
  }

  if (componentOfVar[varId] >= components.size()) {
    // varId is not in an SCC:
    for (const VarId inputId : std::views::keys(graph.inputVars(defInv))) {
      if (!visited[inputId]) {
        visited[inputId] = true;
        partitionIntoLayersUtil(graph, components, inputId, componentOfVar,
                                visited, layerOfVar, layerHasSCC);
      }
      assert(layerOfVar[inputId] < layerHasSCC.size());
      layerOfVar[varId] = std::max(
          layerOfVar[varId],
          layerOfVar[inputId] + (layerHasSCC[layerOfVar[inputId]] ? 1 : 0));
    }
    if (layerOfVar[varId] >= layerHasSCC.size()) {
      // varId is in a new layer, create that layer:
      assert(layerOfVar[varId] == layerHasSCC.size());
      layerHasSCC.emplace_back(false);
    } else {
      // Find the layer that (i) has no SCC and (ii) has an index that equals or
      // is greater to that of varId, creating the layer if it does not exist:
      while (layerHasSCC[layerOfVar[varId]]) {
        ++layerOfVar[varId];
        if (layerOfVar[varId] == layerHasSCC.size()) {
          layerHasSCC.emplace_back(false);
        }
      }
    }
  } else {
    // varId is in an SCC:
    const size_t comp = componentOfVar[varId];
    for (const VarId cVarId : components[comp]) {
      visited[cVarId] = true;
      assert(graph.definingInvariant(cVarId) != NULL_ID);
      for (const VarId inputId : std::views::keys(graph.inputVars(defInv))) {
        if (componentOfVar[inputId] != comp) {
          if (!visited[inputId]) {
            visited[inputId] = true;
            partitionIntoLayersUtil(graph, components, inputId, componentOfVar,
                                    visited, layerOfVar, layerHasSCC);
          }
          assert(layerOfVar[inputId] < layerHasSCC.size());
          // update layer of varId. The layer of the component will be updated
          // below.
          layerOfVar[varId] =
              std::max(layerOfVar[varId], layerOfVar[inputId] + 1);
        }
      }
    }
    if (layerOfVar[varId] >= layerHasSCC.size()) {
      // varId is in a new layer, create that layer:
      assert(layerOfVar[varId] == layerHasSCC.size());
      layerHasSCC.emplace_back(true);
    } else {
      // Find the layer that has (i) an SCC and (ii) an index that equals or is
      // greater to that of varId, creating the layer if it does not exist:
      while (!layerHasSCC[layerOfVar[varId]]) {
        ++layerOfVar[varId];
        if (layerOfVar[varId] == layerHasSCC.size()) {
          layerHasSCC.emplace_back(true);
        }
      }
    }
    // update the layer of the remaining variables in the SCC:
    for (const VarId cVarId : components[comp]) {
      layerOfVar[cVarId] = layerOfVar[varId];
    }
  }
}

static std::vector<bool> partitionIntoLayersUsingSCC(
    const PropagationGraph& graph,
    const std::vector<std::vector<VarId>>& components,
    std::vector<size_t>& layerOfVar) {
  assert(layerOfVar.size() == graph.numVars());
  assert(std::ranges::all_of(layerOfVar,
                             [&](const size_t layer) { return layer == 0; }));

  std::vector<size_t> componentOfVar(graph.numVars(), components.size());
  for (size_t c = 0; c < components.size(); ++c) {
    for (const VarId varId : components[c]) {
      componentOfVar[varId] = c;
    }
  }
  std::vector<bool> visited(graph.numVars(), false);
  std::vector<bool> layerHasSCC;
  layerHasSCC.reserve(components.size() * 2);

  for (const VarId evalVarId : graph.evaluationVars()) {
    partitionIntoLayersUtil(graph, components, evalVarId, componentOfVar,
                            visited, layerOfVar, layerHasSCC);
  }
  for (Int c = static_cast<Int>(components.size()) - 1; c >= 0; --c) {
    for (const VarId varId : components[c]) {
      if (!visited[varId]) {
        partitionIntoLayersUtil(graph, components, varId, componentOfVar,
                                visited, layerOfVar, layerHasSCC);
      }
    }
  }
  return layerHasSCC;
}

static bool hasStaticCycle(const PropagationGraph& graph,
                           const std::vector<VarId>& component,
                           size_t componentIndex,
                           const std::vector<size_t>& componentOfVar) {
  std::vector<VarId> stack;
  std::vector<Int> discoverTime(graph.numVars(), -1);
  stack.reserve(component.size());
  Int time = 0;
  for (const VarId orig : component) {
    if (discoverTime[orig] < 0) {
      continue;
    }
    discoverTime[orig] = time;
    ++time;
    stack.emplace_back(orig);

    while (!stack.empty()) {
      const VarId outputId = stack.back();
      stack.pop_back();
      discoverTime[outputId] = discoverTime[orig];
      const auto defInv = graph.definingInvariant(outputId);
      if (defInv == NULL_ID) {
        continue;
      }
      for (const auto& [inputId, isDynInput] : graph.inputVars(defInv)) {
        if (componentOfVar[inputId] != componentIndex || isDynInput) {
          continue;
        }
        if (discoverTime[inputId] == discoverTime[orig]) {
          return true;
        }
        if (discoverTime[inputId] < 0) {
          stack.emplace_back(inputId);
        }
      }
    }
  }
  return false;
}

static bool hasUndeterminableDynamicCycle(
    const PropagationGraph& graph, const std::vector<VarId>& component,
    size_t componentIndex, const std::vector<size_t>& componentOfVar) {
  for (const VarId outputId : component) {
    const auto defInv = graph.definingInvariant(outputId);
    if (defInv == NULL_ID || !graph.isDynamicInvariant(defInv)) {
      continue;
    }
    bool hasStaticInLayer = false;
    bool hasDynamicInLayer = false;
    // If defInf only has static input in layer, then it is a static invariant
    // in layer.
    //.Else, if defInv only has dynamic input in layer, then it is a dynamic
    // invariant in layer.
    // Otherwise, defInv has a mix of static and dynamic inputs in layer, and
    // creates an undeterminable dynamic cycle.
    for (const auto& [inputId, isDynInput] : graph.inputVars(defInv)) {
      if (componentOfVar[inputId] == componentIndex) {
        hasStaticInLayer = hasStaticInLayer || !isDynInput;
        hasDynamicInLayer = hasDynamicInLayer || isDynInput;
        if (hasStaticInLayer && hasDynamicInLayer) {
          return true;
        }
      }
    }
  }
  return false;
}

void topologicallyOrderUtil(
    const PropagationGraph& graph, const Timestamp ts,
    std::vector<bool>& inFrontier, const VarId varId,
    const std::vector<PropagationGraph::LayerIndex>& varLayerIndex,
    const size_t layerOffset, std::vector<size_t>& topologicalNumber) {
  assert(varId < varLayerIndex.size());
  assert(varLayerIndex.at(varId).layer < graph.numLayers());
  const auto& [layer, index] = varLayerIndex.at(varId);

  // sanity:
  assert(varLayerIndex.at(varId).index < graph.varsInLayer(layer).size());
  assert(layer < graph.numLayers());
  assert(index < graph.varsInLayer(layer).size());
  assert(index < inFrontier.size());
  assert(varId == graph.varsInLayer(layer).at(index));

  if (inFrontier[index]) {
    throw TopologicalOrderError();
  }
  assert(topologicalNumber.at(varId) == graph.numVars());
  if (topologicalNumber[varId] != graph.numVars()) {
    // already visited:
    return;
  }

  // Get defining invariant:
  const InvariantId defInv = graph.definingInvariant(varId);

  // reset the topological number:
  topologicalNumber[varId] = layerOffset;

  if (defInv == NULL_ID) {
    // The current variable is a search variable:
    assert(topologicalNumber[varId] == 0);
    return;
  }

  // add the current variable to the frontier:
  inFrontier[index] = true;

  const bool isDynInv =
      graph.hasDynamicCycle(layer) && graph.isDynamicInvariant(defInv);

  // For any invariant in a layer without an SCC, each input is in the same or a
  // previous layer:
  assert(isDynInv ||
         std::ranges::all_of(std::views::keys(graph.inputVars(defInv)),
                             [&](const VarId inputId) {
                               if (inputId == NULL_ID) {
                                 return false;
                               }
                               return varLayerIndex[inputId].layer <= layer;
                             }));

  // For any dynamic invariant in a layer with an SCC: either (i) all dynamic
  // inputs are in a previous level or (ii) all static inputs are in a previous
  // level
  assert(!isDynInv ||
         std::ranges::all_of(graph.inputVars(defInv),
                             [&](const std::pair<VarId, bool>& p) {
                               if (p.first == NULL_ID) {
                                 return false;
                               }
                               if (p.second) {
                                 return varLayerIndex[p.first].layer < layer;
                               }
                               return true;
                             }) ||
         std::ranges::all_of(graph.inputVars(defInv),
                             [&](const std::pair<VarId, bool>& p) {
                               if (p.first == NULL_ID) {
                                 return false;
                               }
                               if (!p.second) {
                                 return varLayerIndex[p.first].layer < layer;
                               }
                               return true;
                             }));

  const VarId dynInput = isDynInv ? graph.dynamicInputVar(ts, defInv) : NULL_ID;
  const size_t numVars = graph.numVars();
  for (const auto& [inputId, isDynamicInput] : graph.inputVars(defInv)) {
    if (isDynInv && isDynamicInput && dynInput != inputId) {
      continue;
    }
    assert(inputId < varLayerIndex.size());
    const size_t inputLayer = varLayerIndex[inputId].layer;
    const size_t tn = topologicalNumber[inputId];
    if (inputLayer == layer && tn == numVars) {
      assert(!inFrontier.at(varLayerIndex.at(inputId).index));
      topologicallyOrderUtil(graph, ts, inFrontier, inputId, varLayerIndex,
                             layerOffset, topologicalNumber);
      assert(topologicalNumber[inputId] != graph.numVars());
    }
    assert(topologicalNumber[inputId] != graph.numVars());
    topologicalNumber[varId] =
        std::max(topologicalNumber[varId], topologicalNumber[inputId] + 1);
  }
  assert(!isDynInv || dynInput == graph.dynamicInputVar(ts, defInv));
  assert(std::ranges::all_of(
      graph.inputVars(defInv), [&](const std::pair<VarId, bool>& p) {
        if (p.first == NULL_ID) {
          return false;
        }
        if (isDynInv && p.second) {
          if (dynInput == p.first &&
              topologicalNumber[p.first] >= topologicalNumber[varId]) {
            return false;
          }
          return true;
        }
        if (topologicalNumber[p.first] >= topologicalNumber[varId]) {
          return false;
        }
        return true;
      }));

  inFrontier[index] = false;
}

PropagationGraph::PropagationGraph(const Store& store, size_t expectedSize)
    : _store(store) {
  _definingInvariant.reserve(expectedSize);
  _varsDefinedByInvariant.reserve(expectedSize);
  _inputVars.reserve(expectedSize);
  _isDynamicInvariant.reserve(expectedSize);
  _listeningInvariantData.reserve(expectedSize);
  _varLayerIndex.reserve(expectedSize);
  _topologicalNumber.reserve(expectedSize);
}

void PropagationGraph::registerInvariant(
    [[maybe_unused]] InvariantId invariantId) {
  // Everything must be registered in sequence.
  assert(invariantId == _varsDefinedByInvariant.size());
  assert(invariantId == _isDynamicInvariant.size());
  assert(invariantId == _inputVars.size());

  _varsDefinedByInvariant.emplace_back();
  _isDynamicInvariant.emplace_back(false);
  _inputVars.emplace_back();
  ++_numInvariants;
}

void PropagationGraph::registerVar([[maybe_unused]] VarId id) {
  assert(id == _definingInvariant.size());
  assert(id == _listeningInvariantData.size());
  assert(id == _varLayerIndex.size());
  assert(id == _topologicalNumber.size());

  _definingInvariant.emplace_back(NULL_ID);
  _listeningInvariantData.emplace_back();
  _varLayerIndex.emplace_back();
  _topologicalNumber.emplace_back();
  ++_numVars;
}

void PropagationGraph::registerInvariantInput(InvariantId invariantId,
                                              VarId inputVarId, LocalId localId,
                                              bool isDynamicInput) {
  assert(invariantId != NULL_ID && inputVarId != NULL_ID);
  assert(inputVarId < _definingInvariant.size());
  if (_definingInvariant[inputVarId] == invariantId) {
    return;
  }
  assert(invariantId < _isDynamicInvariant.size());
  _isDynamicInvariant[invariantId] =
      _isDynamicInvariant[invariantId] || isDynamicInput;

  assert(inputVarId < _listeningInvariantData.size());
  _listeningInvariantData[inputVarId].emplace_back(invariantId, localId);

  assert(invariantId < _inputVars.size());
  _inputVars[invariantId].emplace_back(inputVarId, isDynamicInput);
}

void PropagationGraph::registerDefinedVar(VarId varId,
                                          InvariantId invariantId) {
  assert(varId != NULL_ID && invariantId != NULL_ID);
  if (_definingInvariant.at(varId) != NULL_ID) {
    throw VarAlreadyDefinedException(
        "Variable " + std::to_string(varId) + " already defined by invariant " +
        std::to_string(_definingInvariant.at(varId)));
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
        _listeningInvariantData[varId].begin() + static_cast<Int>(index));
    assert(std::ranges::all_of(_listeningInvariantData[varId].begin(),
                               _listeningInvariantData[varId].end(),
                               [&](const ListeningInvariantData& data) {
                                 return data.invariantId != invariantId;
                               }));
  }
  _definingInvariant[varId] = invariantId;
  _varsDefinedByInvariant[invariantId].push_back(varId);
}

void PropagationGraph::close(Timestamp ts) {
  _isSearchVar.resize(numVars());
  _isEvaluationVar.resize(numVars());
  _evaluationVars.clear();
  _searchVars.clear();
  for (size_t i = 0; i < numVars(); ++i) {
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
  topologicallyOrder(ts);
  // Reset propagation queue data structure.

  _propagationQueue.init(numVars(), numLayers());
  for (VarId vId = 0; vId < numVars(); ++vId) {
    _propagationQueue.initVar(vId, varPosition(vId));
  }
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
  // Step 1: find all SCCs:
  auto components = SCC(*this);
  // Step 2: check for undeterminable cycles in the SCCs:
  // For any SCC, an undeterminable cycle contains:
  // * a fully static cycle or
  // * a dynamic invariant with both a static and dynamic input also in the SCC
  std::vector<size_t> componentOfVar(numVars(), components.size());
  for (size_t c = 0; c < components.size(); ++c) {
    for (const VarId varId : components[c]) {
      assert(componentOfVar[varId] == components.size());
      componentOfVar[varId] = c;
    }
  }
  for (size_t c = 0; c < components.size(); ++c) {
    if (hasStaticCycle(*this, components[c], c, componentOfVar)) {
      throw PropagationGraphHasCycles(
          "PropagationGraph has one or more static cycles");
    }
    if (hasUndeterminableDynamicCycle(*this, components[c], c,
                                      componentOfVar)) {
      throw PropagationGraphHasCycles(
          "PropagationGraph has one or more bad dynamic cycles");
    }
  }
  componentOfVar.clear();

  // Step 3: using the SCCs, partition into layers:
  std::vector<size_t> layerOfVar(numVars(), 0);
  _layerHasDynamicCycle =
      partitionIntoLayersUsingSCC(*this, components, layerOfVar);
  components.clear();

  assert(layerOfVar.size() == numVars());

  _varLayerIndex.resize(numVars());
  _varsInLayer.assign(_layerHasDynamicCycle.size(), std::vector<VarId>());

  for (VarId varId = 0; varId < numVars(); ++varId) {
    const size_t layer = layerOfVar[varId];
    assert(layer < _varsInLayer.size());
    const size_t index = _varsInLayer[layer].size();
    _varsInLayer[layer].emplace_back(varId);
    _varLayerIndex[varId].layer = layer;
    _varLayerIndex[varId].index = index;
  }

  for (const InvariantId invId : _definingInvariant) {
    if (invId == NULL_ID) {
      continue;
    }
    assert(_store.constInvariant(invId).primaryDefinedVar() != NULL_ID);

    const size_t layer =
        _varLayerIndex.at(_store.constInvariant(invId).primaryDefinedVar())
            .layer;
    assert(std::ranges::all_of(varsDefinedBy(invId), [&](const VarId varId) {
      return _varLayerIndex.at(varId).layer == layer;
    }));
    const bool isDyn =
        _layerHasDynamicCycle.at(layer) && isDynamicInvariant(invId);
    if (!isDyn) {
      assert(std::ranges::all_of(
          std::views::keys(inputVars(invId)), [&](const VarId inputId) {
            return _varLayerIndex.at(inputId).layer <= layer;
          }));
    } else {
      bool allStaticInPrevLayer = true;
      bool allDynInPrevLayer = true;
      for (const auto& [inputId, isDynInput] : inputVars(invId)) {
        const size_t inputLayer = _varLayerIndex.at(inputId).layer;
        assert(inputLayer <= layer);
        if (isDynInput) {
          allDynInPrevLayer = allDynInPrevLayer && inputLayer < layer;
        } else {
          allStaticInPrevLayer = allStaticInPrevLayer && inputLayer < layer;
        }
      }
      assert(allStaticInPrevLayer || allDynInPrevLayer);
    }
  }

  // Step 4: compute layer offsets for topological numbers
  _topologicalNumberOffset.resize(_varsInLayer.size());
  _topologicalNumberOffset[0] = 0;
  for (size_t layer = 1; layer < _varsInLayer.size(); ++layer) {
    _topologicalNumberOffset[layer] =
        _topologicalNumberOffset[layer - 1] + _varsInLayer[layer - 1].size();
  }

  assert(numVars() == _varLayerIndex.size());

  assert(all_in_range(0, numVars(), [&](const VarId varId) {
    const size_t layer = _varLayerIndex.at(varId).layer;
    const size_t index = _varLayerIndex.at(varId).index;

    if (index >= _varsInLayer.at(layer).size()) {
      return false;
    }
    if (varId != _varsInLayer.at(layer).at(index)) {
      return false;
    }

    const auto defInv = definingInvariant(varId);

    if (defInv == NULL_ID) {
      if (layer != 0) {
        return false;
      }
      return layer == 0;
    }

    return std::ranges::all_of(std::views::keys(inputVars(defInv)),
                               [&](const VarId inputId) {
                                 if (_varLayerIndex.at(inputId).layer > layer) {
                                   return false;
                                 }
                                 return true;
                               });
  }));
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
  for (const VarId varId : _varsInLayer[layer]) {
    _topologicalNumber[varId] = numVars();
  }
  std::vector<bool> inFrontier(_varsInLayer[layer].size(), false);
  for (const VarId varId : _varsInLayer[layer]) {
    if (_topologicalNumber[varId] == numVars()) {
      topologicallyOrderUtil(*this, ts, inFrontier, varId, _varLayerIndex,
                             _topologicalNumberOffset[layer],
                             _topologicalNumber);
    }
    assert(_topologicalNumber[varId] < numVars());
  }
  assert(std::ranges::none_of(inFrontier, [&](const bool b) { return b; }));
  for (const VarId outputId : _varsInLayer[layer]) {
    const InvariantId defInv = definingInvariant(outputId);
    if (defInv == NULL_ID) {
      assert(_topologicalNumber[outputId] == 0);
      continue;
    }
    const bool isDynInv =
        _layerHasDynamicCycle.at(layer) && isDynamicInvariant(defInv);

    const VarId dynInput = isDynInv ? dynamicInputVar(ts, defInv) : NULL_ID;
    for (const auto& [inputId, isDynamicInput] : inputVars(defInv)) {
      if (isDynInv && isDynamicInput && dynInput != inputId) {
        continue;
      }
      if (_topologicalNumber[inputId] >= _topologicalNumber[outputId]) {
        throw TopologicalOrderError();
      }
    }
  }

  if (updatePriorityQueue) {
    for (const VarId varId : _varsInLayer[layer]) {
      _propagationQueue.updatePriority(varId, _topologicalNumber[varId]);
    }
  }
}

void PropagationGraph::topologicallyOrder(Timestamp ts) {
  for (size_t layer = 0; layer < numLayers(); ++layer) {
    topologicallyOrder(ts, layer, false);
  }
}
}  // namespace atlantis::propagation

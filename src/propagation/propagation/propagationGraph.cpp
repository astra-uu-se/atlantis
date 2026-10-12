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

inline bool all_in_range(const size_t start, const size_t stop,
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

  for (auto iter = graph.outgoingEdges(inputId).begin();
       iter != graph.outgoingEdges(inputId).end(); ++iter) {
    if (iter->invariantId() == NULL_ID) {
      continue;
    }
    for (const VarId outputId : graph.varsDefinedBy(iter->invariantId())) {
      if (discoverTime[outputId] < 0) {
        SCCUtil(graph, outputId, discoverTime, lowTime, stack, onStack, time,
                components);
        lowTime[inputId] = std::min(lowTime[outputId], lowTime[inputId]);
      } else if (onStack[outputId]) {
        lowTime[inputId] = std::min(lowTime[inputId], discoverTime[outputId]);
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
  assert(
      std::ranges::all_of(discoverTime, [&](const Int i) { return i >= 0; }));
  return components;
}

static void partitionIntoLayersUtil(
    const PropagationGraph& graph,
    const std::vector<std::vector<VarId>>& components, const VarId varId,
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
    for (auto iter = graph.incomingEdges(defInv).begin();
         iter != graph.incomingEdges(defInv).end(); ++iter) {
      const VarId inputId = *iter;
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
      for (auto iter = graph.incomingEdges(defInv).begin();
           iter != graph.incomingEdges(defInv).end(); ++iter) {
        const VarId inputId = *iter;
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
                           const size_t componentIndex,
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
      for (const auto& staticInput : graph.staticInputVars(defInv)) {
        if (componentOfVar[staticInput] != componentIndex) {
          continue;
        }
        if (discoverTime[staticInput] == discoverTime[orig]) {
          return true;
        }
        if (discoverTime[staticInput] < 0) {
          stack.emplace_back(staticInput);
        }
      }
    }
  }
  return false;
}

static bool hasUndeterminableDynamicCycle(
    const PropagationGraph& graph, const std::vector<VarId>& component,
    const size_t componentIndex, const std::vector<size_t>& componentOfVar) {
  for (const VarId outputId : component) {
    const auto defInv = graph.definingInvariant(outputId);
    if (defInv == NULL_ID || !graph.isDynamicInvariant(defInv)) {
      continue;
    }
    // If defInf only has static input in layer, then it is a static invariant
    // in layer.
    //.Else, if defInv only has dynamic input in layer, then it is a dynamic
    // invariant in layer.
    // Otherwise, defInv has a mix of static and dynamic inputs in layer, and
    // creates an undeterminable dynamic cycle.
    const bool hasStaticInLayer = std::ranges::any_of(
        graph.staticInputVars(defInv), [&](const VarId varId) {
          return componentOfVar[varId] == componentIndex;
        });
    const bool hasDynamicInLayer = std::ranges::any_of(
        graph.dynamicInputVars(defInv),
        [&](const invariant::IncomingDynamicEdge& edge) {
          return componentOfVar[edge.varId] == componentIndex;
        });
    if (hasStaticInLayer && hasDynamicInLayer) {
      return true;
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
    for (const auto& outgoingEdge :
         graph.outgoingEdges(varId).dynamicEdges().edges()) {
      for (const VarId staticSiblingId :
           graph.staticInputVars(outgoingEdge.invariantId())) {
        const size_t inputLayer = varLayerIndex[staticSiblingId].layer;
        const size_t tn = topologicalNumber[staticSiblingId];
        if (inputLayer == layer && tn == graph.numVars()) {
          assert(!inFrontier.at(varLayerIndex.at(staticSiblingId).index));
          topologicallyOrderUtil(graph, ts, inFrontier, staticSiblingId,
                                 varLayerIndex, layerOffset, topologicalNumber);
          assert(topologicalNumber[staticSiblingId] != graph.numVars());
        }
        topologicalNumber[varId] = std::max(
            topologicalNumber[varId], topologicalNumber[staticSiblingId] + 1);
      }
    }
    return;
  }

  // add the current variable to the frontier:
  inFrontier[index] = true;

  const bool isDynInv =
      graph.hasDynamicCycle(layer) && graph.isDynamicInvariant(defInv);

  // For any invariant in a layer without an SCC, each input is in the same or a
  // previous layer:
  assert(isDynInv || std::all_of(graph.incomingEdges(defInv).begin(),
                                 graph.incomingEdges(defInv).end(),
                                 [&](const VarId inputId) {
                                   return inputId != NULL_ID &&
                                          varLayerIndex[inputId].layer <= layer;
                                 }));

  // For any dynamic invariant in a layer with an SCC: either (i) all dynamic
  // inputs are in a previous level or (ii) all static inputs are in a previous
  // level
  assert(!isDynInv ||
         (std::ranges::all_of(graph.staticInputVars(defInv),
                              [&](const VarId staticInputId) {
                                return staticInputId != NULL_ID &&
                                       varLayerIndex[staticInputId].layer <
                                           layer;
                              }) ||
          std::ranges::all_of(graph.dynamicInputVars(defInv),
                              [&](const invariant::IncomingDynamicEdge& edge) {
                                return edge.varId != NULL_ID &&
                                       varLayerIndex[edge.varId].layer < layer;
                              })));

  const size_t numVars = graph.numVars();
  for (const VarId staticInputId : graph.staticInputVars(defInv)) {
    assert(staticInputId < varLayerIndex.size());
    const size_t inputLayer = varLayerIndex[staticInputId].layer;
    const size_t tn = topologicalNumber[staticInputId];
    if (inputLayer == layer && tn == numVars) {
      assert(!inFrontier.at(varLayerIndex.at(staticInputId).index));
      topologicallyOrderUtil(graph, ts, inFrontier, staticInputId,
                             varLayerIndex, layerOffset, topologicalNumber);
      assert(topologicalNumber[staticInputId] != graph.numVars());
    }
    assert(topologicalNumber[staticInputId] != graph.numVars());
    topologicalNumber[varId] = std::max(topologicalNumber[varId],
                                        topologicalNumber[staticInputId] + 1);
  }
  for (const auto& outgoingEdge :
       graph.outgoingEdges(varId).dynamicEdges().edges()) {
    if (outgoingEdge.invariantId() == NULL_ID) {
      continue;
    }
    for (const VarId staticSiblingId :
         graph.staticInputVars(outgoingEdge.invariantId())) {
      const size_t inputLayer = varLayerIndex[staticSiblingId].layer;
      const size_t tn = topologicalNumber[staticSiblingId];
      if (inputLayer == layer && tn == numVars) {
        assert(!inFrontier.at(varLayerIndex.at(staticSiblingId).index));
        topologicallyOrderUtil(graph, ts, inFrontier, staticSiblingId,
                               varLayerIndex, layerOffset, topologicalNumber);
        assert(topologicalNumber[staticSiblingId] != graph.numVars());
      }
      topologicalNumber[varId] = std::max(
          topologicalNumber[varId], topologicalNumber[staticSiblingId] + 1);
    }
  }
  const VarId dynInput = isDynInv ? graph.dynamicInputVar(ts, defInv) : NULL_ID;
  for (const invariant::IncomingDynamicEdge dynamicEdge :
       graph.dynamicInputVars(defInv)) {
    if (isDynInv && dynInput != dynamicEdge.varId) {
      continue;
    }
    assert(dynamicEdge.varId < varLayerIndex.size());
    const size_t inputLayer = varLayerIndex[dynamicEdge.varId].layer;
    const size_t tn = topologicalNumber[dynamicEdge.varId];
    if (inputLayer == layer && tn == numVars) {
      assert(!inFrontier.at(varLayerIndex.at(dynamicEdge.varId).index));
      topologicallyOrderUtil(graph, ts, inFrontier, dynamicEdge.varId,
                             varLayerIndex, layerOffset, topologicalNumber);
      assert(topologicalNumber[dynamicEdge.varId] != graph.numVars());
    }
    assert(topologicalNumber[dynamicEdge.varId] != graph.numVars());
    topologicalNumber[varId] = std::max(
        topologicalNumber[varId], topologicalNumber[dynamicEdge.varId] + 1);
  }
  assert(!isDynInv || dynInput == graph.dynamicInputVar(ts, defInv));
  assert(std::ranges::all_of(
      graph.staticInputVars(defInv), [&](const VarId inputId) {
        if (inputId == NULL_ID) {
          return false;
        }
        if (topologicalNumber[inputId] >= topologicalNumber[varId]) {
          return false;
        }
        return true;
      }));
  assert(std::ranges::all_of(
      graph.dynamicInputVars(defInv),
      [&](const invariant::IncomingDynamicEdge& edge) {
        if (edge.varId == NULL_ID) {
          return false;
        }
        if (isDynInv) {
          if (dynInput == edge.varId &&
              topologicalNumber[edge.varId] >= topologicalNumber[varId]) {
            return false;
          }
          return true;
        }
        if (topologicalNumber[edge.varId] >= topologicalNumber[varId]) {
          return false;
        }
        return true;
      }));

  inFrontier[index] = false;
}

PropagationGraph::ListeningInvariantData::ListeningInvariantData(
    const InvariantId t_invariantId, const LocalId t_localId)
    : invariantId(t_invariantId), localId(t_localId) {}
PropagationGraph::ListeningInvariantData&
PropagationGraph::ListeningInvariantData::operator=(
    ListeningInvariantData&& other) noexcept {
  invariantId = other.invariantId;
  localId = other.localId;
  return *this;
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

  assert(std::ranges::all_of(_definingInvariant, [&](const InvariantId invId) {
    if (invId == NULL_ID) {
      return true;
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
      assert(isDyn ||
             std::all_of(incomingEdges(invId).begin(),
                         incomingEdges(invId).end(), [&](const VarId inputId) {
                           return _varLayerIndex.at(inputId).layer <= layer;
                         }));
    } else {
      assert(std::ranges::all_of(
          staticInputVars(invId), [&](const VarId staticInputId) {
            return _varLayerIndex.at(staticInputId).layer <= layer;
          }));
      assert(std::ranges::all_of(
          dynamicInputVars(invId),
          [&](const invariant::IncomingDynamicEdge& edge) {
            return _varLayerIndex.at(edge.varId).layer <= layer;
          }));
    }
    return true;
  }));

  // Step 4: compute layer offsets for topological numbers
  _topologicalNumberOffset.resize(_varsInLayer.size());
  if (!_varsInLayer.empty()) {
    _topologicalNumberOffset[0] = 0;
  }
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

    return std::all_of(incomingEdges(defInv).begin(),
                       incomingEdges(defInv).end(), [&](const VarId inputId) {
                         return _varLayerIndex.at(inputId).layer <= layer;
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
void PropagationGraph::topologicallyOrder(const Timestamp ts,
                                          const size_t layer,
                                          const bool updatePriorityQueue) {
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
      assert(_topologicalNumber[outputId] < _varsInLayer.front().size());
      continue;
    }
    const bool isDynInv =
        _layerHasDynamicCycle.at(layer) && isDynamicInvariant(defInv);

    const size_t staticInputTopologicalNumber =
        staticInputVars(defInv).empty()
            ? 0
            : _topologicalNumber[std::ranges::min(
                  staticInputVars(defInv),
                  [&](const VarId lhs, const VarId rhs) {
                    return _topologicalNumber[lhs] < _topologicalNumber[rhs];
                  })];

    if (staticInputTopologicalNumber >= _topologicalNumber[outputId]) {
      throw TopologicalOrderError();
    }
    const bool allStaticBeforeDynamic =
        dynamicInputVars(defInv).empty() ||
        std::ranges::all_of(dynamicInputVars(defInv),
                            [&](const invariant::IncomingDynamicEdge& edge) {
                              return staticInputTopologicalNumber <
                                     _topologicalNumber[edge.varId];
                            });
    const bool allDynamicBeforeStatic =
        !dynamicInputVars(defInv).empty() &&
        std::ranges::all_of(dynamicInputVars(defInv),
                            [&](const invariant::IncomingDynamicEdge& edge) {
                              return _topologicalNumber[edge.varId] <
                                     staticInputTopologicalNumber;
                            });
    if (!allStaticBeforeDynamic && !allDynamicBeforeStatic) {
      throw TopologicalOrderError();
    }
    if (isDynInv) {
      if (_topologicalNumber[dynamicInputVar(ts, defInv)] >=
          _topologicalNumber[outputId]) {
        throw TopologicalOrderError();
      }
    } else {
      for (const auto [dynamicInputId, _] : dynamicInputVars(defInv)) {
        if (_topologicalNumber[dynamicInputId] >=
            _topologicalNumber[outputId]) {
          throw TopologicalOrderError();
        }
      }
    }
  }

  if (updatePriorityQueue) {
    for (const VarId varId : _varsInLayer[layer]) {
      _propagationQueue.updatePriority(varId, _topologicalNumber[varId]);
    }
  }
}

void PropagationGraph::topologicallyOrder(const Timestamp ts) {
  for (size_t layer = 0; layer < numLayers(); ++layer) {
    topologicallyOrder(ts, layer, false);
  }
}

bool PropagationGraph::PriorityCmp::operator()(const VarId left,
                                               const VarId right) const {
  return graph.varPosition(left) > graph.varPosition(right);
}
PropagationGraph::PropagationGraph(const Store& store,
                                   const size_t expectedSize)
    : _store(store) {
  _definingInvariant.reserve(expectedSize);
  _varsDefinedByInvariant.reserve(expectedSize);
  _outgoingEdges.reserve(expectedSize);
  _varLayerIndex.reserve(expectedSize);
  _topologicalNumber.reserve(expectedSize);
}
void PropagationGraph::close(const Timestamp ts) {
  _isSearchVar.resize(numVars());
  _isEvaluationVar.resize(numVars());
  _evaluationVars.clear();
  _searchVars.clear();
  for (size_t i = 0; i < numVars(); ++i) {
    _isEvaluationVar[i] = _outgoingEdges.at(i).empty();
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

  _propagationQueue.init();
  for (VarId vId = 0; vId < numVars(); ++vId) {
    _propagationQueue.initVar(vId, varPosition(vId));
  }
}
void PropagationGraph::registerInvariant(
    [[maybe_unused]] const InvariantId invariantId) {
  // Everything must be registered in sequence.
  assert(invariantId == _varsDefinedByInvariant.size());
  assert(invariantId == _incomingEdges.size());

  _varsDefinedByInvariant.emplace_back();
  _incomingEdges.emplace_back();
  ++_numInvariants;
}
void PropagationGraph::registerVar([[maybe_unused]] const VarId id) {
  assert(id == _definingInvariant.size());
  assert(id == _varLayerIndex.size());
  assert(id == _topologicalNumber.size());
  assert(id == _outgoingEdges.size());

  _definingInvariant.emplace_back(NULL_ID);
  _varLayerIndex.emplace_back();
  _topologicalNumber.emplace_back();
  _outgoingEdges.emplace_back();
  ++_numVars;
}
LocalId PropagationGraph::registerInvariantInput(const InvariantId invariantId,
                                                 const VarId inputId,
                                                 const bool isDynamic) {
  assert(invariantId != NULL_ID && inputId != NULL_ID);
  const bool selfCycle = _definingInvariant[inputId] == invariantId;
  if (selfCycle) {
  }
  if (isDynamic) {
    const LocalId localId = _incomingEdges[invariantId].emplaceDynamic(
        selfCycle ? NULL_ID : inputId,
        selfCycle ? 0 : _outgoingEdges[inputId].dynamicEdges().size());
    if (!selfCycle) {
      _outgoingEdges[inputId].emplaceDynamic(localId, invariantId);
      assert(
          _outgoingEdges[inputId].dynamicEdges().edges().back().invariantId() ==
          invariantId);
      assert(_outgoingEdges[inputId].dynamicEdges().edges().back().localId() ==
             localId);
      assert(_incomingEdges[invariantId].dynamicEdges().at(localId).varId ==
             inputId);
      assert(_incomingEdges[invariantId]
                 .dynamicEdges()
                 .at(localId)
                 .outgoingDynamicEdgeIndex ==
             _outgoingEdges[inputId].dynamicEdges().size() - 1);
    }
    return localId;
  }
  if (!selfCycle) {
    const LocalId localId = _incomingEdges[invariantId].emplaceStatic(inputId);
    _outgoingEdges[inputId].emplaceStatic(localId, invariantId);
    return localId;
  }
  return NULL_ID;
}

void PropagationGraph::makeDynamicInputActive(const Timestamp ts,
                                              const InvariantId invariantId,
                                              const LocalId localId) {
  assert(invariantId != NULL_ID);
  assert(localId < _incomingEdges[invariantId].dynamicEdges().size());
  const auto& inputVarData =
      _incomingEdges[invariantId].dynamicEdges().at(localId);
  _outgoingEdges[inputVarData.varId].makeDynamicActive(
      ts, inputVarData.outgoingDynamicEdgeIndex);
}

void PropagationGraph::makeAllDynamicInputsInactive(
    const Timestamp ts, const InvariantId invariantId) {
  for (const auto& inputVarData : _incomingEdges[invariantId].dynamicEdges()) {
    if (inputVarData.varId == NULL_ID) {
      continue;
    }
    assert(inputVarData.varId < _outgoingEdges.size());
    if (_outgoingEdges[inputVarData.varId].dynamicEdges().isNullId(
            inputVarData.outgoingDynamicEdgeIndex)) {
      continue;
    }
    assert(_outgoingEdges[inputVarData.varId]
               .dynamicEdges()
               .edges()
               .at(inputVarData.outgoingDynamicEdgeIndex)
               .invariantId() == invariantId);
    _outgoingEdges[inputVarData.varId].makeDynamicInactive(
        ts, inputVarData.outgoingDynamicEdgeIndex);
  }
}

void PropagationGraph::makeDynamicInputInactive(const Timestamp ts,
                                                const InvariantId invariantId,
                                                const LocalId localId) {
  assert(invariantId != NULL_ID);
  assert(localId < _incomingEdges[invariantId].dynamicEdges().size());
  const auto& inputVarData =
      _incomingEdges[invariantId].dynamicEdges()[localId];
  if (inputVarData.varId == NULL_ID) {
    return;
  }
  assert(inputVarData.varId < _outgoingEdges.size());
  if (_outgoingEdges[inputVarData.varId].dynamicEdges().isNullId(
          inputVarData.outgoingDynamicEdgeIndex)) {
    return;
  }
  assert(_outgoingEdges[inputVarData.varId]
             .dynamicEdges()
             .edges()
             .at(inputVarData.outgoingDynamicEdgeIndex)
             .invariantId() == invariantId);
  _outgoingEdges[inputVarData.varId].makeDynamicInactive(
      ts, inputVarData.outgoingDynamicEdgeIndex);
}

void PropagationGraph::makeAllDynamicInactive(const Timestamp ts,
                                              const VarId varId) {
  assert(varId != NULL_ID);
  _outgoingEdges[varId].makeAllDynamicInactive(ts);
}

void PropagationGraph::commitOutgoingEdges(const Timestamp ts,
                                           const VarId varId) {
  assert(varId != NULL_ID);
  _outgoingEdges[varId].commitIf(ts);
}

void PropagationGraph::registerDefinedVar(const VarId varId,
                                          const InvariantId invariantId) {
  assert(varId != NULL_ID && invariantId != NULL_ID);
  if (_definingInvariant.at(varId) != NULL_ID) {
    throw VarAlreadyDefinedException(
        "Variable " + std::to_string(varId) + " already defined by invariant " +
        std::to_string(_definingInvariant.at(varId)));
  }
  std::vector<size_t> indicesToRemove;
  for (size_t i = 0; i < _outgoingEdges[varId].staticEdges().size(); ++i) {
    if (_outgoingEdges[varId].staticEdge(i).invariantId() == invariantId) {
      indicesToRemove.emplace_back(i);
    }
  }
  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    _outgoingEdges[varId].eraseStatic(indicesToRemove[i]);
  }
  indicesToRemove.clear();
  size_t i = 0;
  for (const auto& inputData : _outgoingEdges[varId].dynamicEdges().edges()) {
    if (inputData.invariantId() == invariantId) {
      indicesToRemove.emplace_back(i);
    }
    ++i;
  }
  for (const size_t index : indicesToRemove) {
    _outgoingEdges[varId].setDynamicNullId(index);
  }
  _definingInvariant[varId] = invariantId;
  _varsDefinedByInvariant[invariantId].emplace_back(varId);
}

void PropagationGraph::topologicallyOrder(const Timestamp ts,
                                          const size_t layer) {
  topologicallyOrder(ts, layer, true);
}
size_t PropagationGraph::numVars() const {
  return _numVars;  // this ignores null var
}
size_t PropagationGraph::numInvariants() const {
  return _numInvariants;  // this ignores null invariant
}
bool PropagationGraph::isEvaluationVar(const VarId id) const {
  assert(id < _isEvaluationVar.size());
  return _isEvaluationVar[id];
}
bool PropagationGraph::isSearchVar(const VarId id) const {
  assert(id < _isSearchVar.size());
  return _isSearchVar.at(id);
}
bool PropagationGraph::isDynamicInvariant(const InvariantId invariantId) const {
  assert(invariantId < _incomingEdges.size());
  return !_incomingEdges[invariantId].dynamicEdges().empty();
}
InvariantId PropagationGraph::definingInvariant(const VarId varId) const {
  assert(varId < _definingInvariant.size());
  return _definingInvariant[varId];
}
const std::vector<VarId>& PropagationGraph::varsDefinedBy(
    const InvariantId invariantId) const {
  return _varsDefinedByInvariant.at(invariantId);
}

const invariant::IncomingEdgeContainer& PropagationGraph::incomingEdges(
    const InvariantId invId) const {
  assert(invId < _incomingEdges.size());
  return _incomingEdges[invId];
}

const var::OutgoingEdgeContainer& PropagationGraph::outgoingEdges(
    const VarId varId) const {
  assert(varId < _outgoingEdges.size());
  return _outgoingEdges[varId];
}

const std::vector<VarId>& PropagationGraph::staticInputVars(
    const InvariantId invariantId) const {
  assert(invariantId < _incomingEdges.size());
  return _incomingEdges[invariantId].staticEdges();
}

VarId PropagationGraph::dynamicInputVar(
    const Timestamp ts, const InvariantId invariantId) const noexcept {
  return _store.dynamicInputVar(ts, invariantId);
}

const std::vector<invariant::IncomingDynamicEdge>&
PropagationGraph::dynamicInputVars(const InvariantId invariantId) const {
  assert(invariantId < _incomingEdges.size());
  return _incomingEdges[invariantId].dynamicEdges();
}

const std::vector<VarId>& PropagationGraph::searchVars() const {
  return _searchVars;
}
const std::vector<VarId>& PropagationGraph::evaluationVars() const {
  return _evaluationVars;
}
void PropagationGraph::clearPropagationQueue() {
  while (!_propagationQueue.empty()) {
    _propagationQueue.pop();
  }
}
bool PropagationGraph::propagationQueueEmpty() const {
  return _propagationQueue.empty();
}
VarId PropagationGraph::dequeuePropagationQueue() {
  const VarId id = _propagationQueue.top();
  _propagationQueue.pop();
  return id;
}
bool PropagationGraph::hasDynamicCycle() const noexcept {
  return _hasDynamicCycle;
}
bool PropagationGraph::hasDynamicCycle(const size_t layer) const {
  assert(layer < _layerHasDynamicCycle.size());
  return _layerHasDynamicCycle[layer];
}
size_t PropagationGraph::numLayers() const noexcept {
  return _varsInLayer.size();
}
size_t PropagationGraph::numVarsInLayer(const size_t layer) const noexcept {
  assert(layer < numLayers());
  return _varsInLayer[layer].size();
}

const std::vector<VarId>& PropagationGraph::varsInLayer(
    const size_t layer) const noexcept {
  return _varsInLayer[layer];
}

size_t PropagationGraph::varLayer(const VarId id) const {
  return _varLayerIndex.at(id).layer;
}
size_t PropagationGraph::invariantLayer(const InvariantId invariantId) const {
  assert(!varsDefinedBy(invariantId).empty());
  return _varLayerIndex.at(varsDefinedBy(invariantId).front()).layer;
}
size_t PropagationGraph::varPosition(const VarId id) const {
  return _topologicalNumber[id];
}

size_t PropagationGraph::invariantPosition(
    const InvariantId invariantId) const {
  assert(!_varsDefinedByInvariant.at(invariantId).empty());
  return _topologicalNumber.at(_varsDefinedByInvariant.at(invariantId).front());
}

void PropagationGraph::enqueuePropagationQueue(const VarId id) {
  _propagationQueue.push(id);
}
}  // namespace atlantis::propagation

#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <queue>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "misc/logging.hpp"
#include "propagation/store/store.hpp"
#include "propagation/utils/idMap.hpp"
#include "propagationQueue.hpp"
#include "types.hpp"

namespace atlantis::propagation {

class PropagationGraph {
 public:
  struct ListeningInvariantData {
   public:
    InvariantId invariantId;
    LocalId localId;
    ListeningInvariantData(const ListeningInvariantData& other) = default;
    ListeningInvariantData(const InvariantId t_invariantId,
                           const LocalId t_localId)
        : invariantId(t_invariantId), localId(t_localId) {}
    ListeningInvariantData& operator=(ListeningInvariantData&& other) noexcept {
      invariantId = other.invariantId;
      localId = other.localId;
      return *this;
    }
  };

 private:
  std::vector<bool> _isEvaluationVar{};
  std::vector<bool> _isSearchVar{};
  std::vector<VarIdBase> _searchVars{};
  std::vector<VarIdBase> _evaluationVars{};

  const Store& _store;

  /**
   * Map from VarID -> InvariantId
   *
   * Maps to nullptr if not defined by any invariant.
   */
  IdMap<VarIdBase, InvariantId> _definingInvariant;

  /**
   * Map from InvariantId -> list of VarId
   *
   * Maps an invariant to all variables it defines.
   */
  IdMap<InvariantId, std::vector<VarIdBase>> _varsDefinedByInvariant;
  /**
   * Map from InvariantId -> list of VarId
   *
   * Maps an invariant to all its variable inputs.
   */
  // Given input variable with VarId x and invariant with InvariantId i, then
  // _inputVars[i] = <x, b>, where b = true iff x is a dynamic input
  // to i.
  IdMap<InvariantId, std::vector<std::pair<VarIdBase, bool>>> _inputVars;

  // Given invariant with InvariantId i, _isDynamicInvairiant[i] = true iff i
  // has one or more dynamic input variables.
  IdMap<InvariantId, bool> _isDynamicInvariant;

  // Map from VarID -> vector of InvariantID
  IdMap<VarIdBase, std::vector<ListeningInvariantData>> _listeningInvariantData;

  std::vector<std::vector<VarIdBase>> _varsInLayer;
  struct LayerIndex {
    size_t layer;
    size_t index;
  };
  IdMap<VarIdBase, LayerIndex> _varLayerIndex;
  IdMap<VarIdBase, size_t> _varPosition;
  std::vector<size_t> _layerPositionOffset{};
  std::vector<bool> _layerHasDynamicCycle{};
  bool _hasDynamicCycle{false};
  size_t _numInvariants{0};
  size_t _numVars{0};

  bool containsStaticCycle(std::vector<bool>& visited,
                           std::vector<bool>& inFrontier, VarIdBase varId);
  bool containsStaticCycle();
  void partitionIntoLayers(std::vector<bool>& visited, VarIdBase varId);
  void partitionIntoLayers();
  bool containsDynamicCycle(std::vector<bool>& visited, VarIdBase varId);
  bool containsDynamicCycle(size_t level);
  void mergeLayersWithoutDynamicCycles();
  void computeLayerOffsets();
  void topologicallyOrder(Timestamp ts, std::vector<bool>& inFrontier,
                          VarIdBase varId);
  void topologicallyOrder(Timestamp ts, size_t layer, bool updatePriorityQueue);
  void topologicallyOrder(Timestamp ts);

  struct PriorityCmp {
    PropagationGraph& graph;
    explicit PriorityCmp(PropagationGraph& g) : graph(g) {}
    bool operator()(VarIdBase left, VarIdBase right) {
      return graph.position(left) > graph.position(right);
    }
  };

  PropagationQueue _propagationQueue;

  [[nodiscard]] inline VarId dynamicInputVar(
      Timestamp ts, InvariantId invariantId) const noexcept {
    return _store.dynamicInputVar(ts, invariantId);
  }

 public:
  explicit PropagationGraph(const Store& store, size_t expectedSize = 1000u);

  /**
   * update internal datastructures based on currently registered  variables and
   * invariants.
   */
  void close(Timestamp ts);

  /**
   * Register an invariant in the propagation graph.
   */
  void registerInvariant(InvariantId);

  /**
   * Register a variable in the propagation graph.
   */
  void registerVar(VarIdBase);

  /**
   * Register that inputId is a input of invariantId
   * @param invariantId the invariant
   * @param inputId the variable input
   * @param isDynamic true if the variable is a dynamic input to the invariant.
   */
  void registerInvariantInput(InvariantId invariantId, VarIdBase inputId,
                              LocalId localId, bool isDynamic);

  /**
   * Register that source functionally defines varId
   * @param varId the variable that is defined by the invariant
   * @param invriant the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVar(VarIdBase varId, InvariantId invariant);

  /**
   * @brief topologically orders the layer
   *
   * @param ts
   * @param layer
   */
  inline void topologicallyOrder(Timestamp ts, size_t layer) {
    topologicallyOrder(ts, layer, true);
  }

  [[nodiscard]] inline size_t numVars() const {
    return _numVars;  // this ignores null var
  }

  [[nodiscard]] inline size_t numInvariants() const {
    return _numInvariants;  // this ignores null invariant
  }

  inline bool isEvaluationVar(VarIdBase id) {
    assert(id < _isEvaluationVar.size());
    return _isEvaluationVar[id];
  }

  inline bool isSearchVar(VarIdBase id) {
    assert(id < _isSearchVar.size());
    return _isSearchVar.at(id);
  }

  [[nodiscard]] inline bool isDynamicInvariant(InvariantId id) const {
    return _isDynamicInvariant.get(id);
  }

  [[nodiscard]] inline InvariantId definingInvariant(VarIdBase id) const {
    // Returns NULL_ID if id is a search variable (not defined by an invariant)
    return _definingInvariant.at(id);
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& varsDefinedBy(
      InvariantId invariantId) const {
    return _varsDefinedByInvariant.at(invariantId);
  }

  [[nodiscard]] inline const std::vector<ListeningInvariantData>&
  listeningInvariantData(VarId id) const {
    return _listeningInvariantData.at(id.id);
  }

  [[nodiscard]] inline const std::vector<std::pair<VarIdBase, bool>>& inputVars(
      InvariantId invariantId) const {
    return _inputVars.at(invariantId);
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& searchVars() const {
    return _searchVars;
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& evaluationVars() const {
    return _evaluationVars;
  }

  inline void clearPropagationQueue() {
    while (!_propagationQueue.empty()) {
      _propagationQueue.pop();
    }
  }

  [[nodiscard]] inline bool propagationQueueEmpty() {
    return _propagationQueue.empty();
  }

  [[nodiscard]] inline VarIdBase dequeuePropagationQueue() {
    VarIdBase id = _propagationQueue.top();
    _propagationQueue.pop();
    return id;
  }

  [[nodiscard]] bool hasDynamicCycle() const noexcept {
    return _hasDynamicCycle;
  }

  [[nodiscard]] bool hasDynamicCycle(size_t layer) const {
    assert(layer < _layerHasDynamicCycle.size());
    return _layerHasDynamicCycle[layer];
  }

  [[nodiscard]] inline size_t numLayers() const noexcept {
    return _varsInLayer.size();
  }

  [[nodiscard]] inline size_t numVarsInLayer(size_t layer) const noexcept {
    assert(layer < numLayers());
    return _varsInLayer[layer].size();
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& varsInLayer(
      size_t layer) const noexcept {
    return _varsInLayer[layer];
  }

  [[nodiscard]] inline size_t layer(VarIdBase id) {
    return _varLayerIndex.at(id).layer;
  }

  [[nodiscard]] inline size_t layer(InvariantId invariantId) {
    assert(!varsDefinedBy(invariantId).empty());
    return _varLayerIndex.at(varsDefinedBy(invariantId).front()).layer;
  }

  [[nodiscard]] inline size_t position(VarIdBase id) {
    return _varPosition[id];
  }

  inline size_t position(InvariantId invariantId) {
    assert(!_varsDefinedByInvariant.at(invariantId).empty());
    return _varPosition.at(_varsDefinedByInvariant.at(invariantId).front());
  }

  inline void enqueuePropagationQueue(VarIdBase id) {
    _propagationQueue.push(id);
  }
};

}  // namespace atlantis::propagation
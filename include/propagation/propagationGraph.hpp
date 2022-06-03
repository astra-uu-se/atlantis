#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <queue>
#include <vector>

#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "layeredPropagationQueue.hpp"
#include "propagationQueue.hpp"
#include "store/store.hpp"
#include "utils/idMap.hpp"

class PropagationGraph {
 private:
  std::vector<bool> _isEvaluationVariable{};
  std::vector<bool> _isSearchVariable{};
  std::vector<VarIdBase> _searchVariables{};
  std::vector<VarIdBase> _evaluationVariables{};

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
  IdMap<InvariantId, std::vector<VarIdBase>> _variablesDefinedByInvariant;
  /**
   * Map from InvariantId -> list of VarId
   *
   * Maps an invariant to all its variable inputs.
   */
  // Given input variable with VarId x and invariant with InvariantId i, then
  // _inputVariables[i] = <x, b>, where b = true iff x is a dynamic input
  // to i.
  IdMap<InvariantId, std::vector<std::pair<VarIdBase, bool>>> _inputVariables;

  // Given invariant with InvariantId i, _isDynamicInvairiant[i] = true iff i
  // has one or more dynamic input variables.
  IdMap<InvariantId, bool> _isDynamicInvariant;

  // Map from VarId -> vector of InvariantId
  IdMap<VarIdBase, std::vector<InvariantId>> _listeningInvariants;

  std::vector<std::vector<VarIdBase>> _variablesInLayer;
  struct LayerIndex {
    size_t layer;
    size_t index;
  };
  IdMap<VarIdBase, LayerIndex> _variableLayerIndex;
  IdMap<VarIdBase, size_t> _variablePosition;
  std::vector<size_t> _layerPositionOffset{};
  std::vector<bool> _layerHasDynamicCycle{};
  bool _hasDynamicCycle{false};
  size_t _numInvariants{0};
  size_t _numVariables{0};

  bool containsStaticCycle(std::vector<bool>& visited,
                           std::vector<bool>& inPenumbra, VarIdBase varId);
  bool containsStaticCycle();
  void partitionIntoLayers(std::vector<bool>& visited, VarIdBase varId);
  void partitionIntoLayers();
  bool containsDynamicCycle(std::vector<bool>& visited,
                            std::vector<bool>& inPenumbra, VarIdBase varId);
  bool containsDynamicCycle(size_t level);
  void mergeLayersWithoutDynamicCycles();
  void computeLayerOffsets();
  size_t topologicallyOrder(Timestamp ts, VarIdBase varId, size_t curPosition);
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

  [[nodiscard]] inline VarId dynamicInputVariable(Timestamp ts,
                                                  InvariantId invariantId) {
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
                              bool isDynamic);

  /**
   * Register that source functionally defines varId
   * @param varId the variable that is defined by the invariant
   * @param invriant the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarIdBase varId, InvariantId invariant);

  /**
   * @brief topologically orders the layer
   *
   * @param ts
   * @param layer
   */
  inline void topologicallyOrder(Timestamp ts, size_t layer) {
    topologicallyOrder(ts, layer, true);
  }

  [[nodiscard]] inline size_t numVariables() const {
    return _numVariables;  // this ignores null var
  }

  [[nodiscard]] inline size_t numInvariants() const {
    return _numInvariants;  // this ignores null invariant
  }

  inline bool isEvaluationVariable(VarIdBase id) {
    assert(id < _isEvaluationVariable.size());
    return _isEvaluationVariable[id];
  }

  inline bool isSearchVariable(VarIdBase id) {
    assert(id < _isSearchVariable.size());
    return _isSearchVariable.at(id);
  }

  inline bool isDynamicInvariant(InvariantId id) const {
    return _isDynamicInvariant.get(id);
  }

  inline InvariantId definingInvariant(VarIdBase id) {
    // Returns NULL_ID if id is a search variable (not defined by an invariant)
    return _definingInvariant.at(id);
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& variablesDefinedBy(
      InvariantId invariantId) const {
    return _variablesDefinedByInvariant.at(invariantId);
  }

  [[nodiscard]] inline const std::vector<InvariantId>& listeningInvariants(
      VarId id) const {
    return _listeningInvariants.at(id);
  }

  [[nodiscard]] inline const std::vector<std::pair<VarIdBase, bool>>&
  inputVariables(InvariantId invariantId) const {
    return _inputVariables.at(invariantId);
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& searchVariables() const {
    return _searchVariables;
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& evaluationVariables()
      const {
    return _evaluationVariables;
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
    return _variablesInLayer.size();
  }

  [[nodiscard]] inline size_t layer(VarIdBase id) {
    return _variableLayerIndex.at(id).layer;
  }

  [[nodiscard]] inline size_t layer(InvariantId invariantId) {
    assert(!variablesDefinedBy(invariantId).empty());
    return _variableLayerIndex.at(variablesDefinedBy(invariantId).front())
        .layer;
  }

  [[nodiscard]] inline size_t position(VarIdBase id) {
    return _variablePosition[id];
  }

  inline size_t position(InvariantId invariantId) {
    assert(!_variablesDefinedByInvariant.at(invariantId).empty());
    return _variablePosition.at(
        _variablesDefinedByInvariant.at(invariantId).front());
  }

  inline void enqueuePropagationQueue(VarIdBase id) {
    _propagationQueue.push(id);
  }
};

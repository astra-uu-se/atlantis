#pragma once

#include <vector>

#include "atlantis/propagation/arcs.hpp"
#include "atlantis/propagation/propagation/propagationQueue.hpp"
#include "atlantis/propagation/store/store.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class PropagationGraph {
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
  // _staticInputVars[i] = <x, b>, where b = true iff x is a dynamic input
  // to i.
  IdMap<InvariantId, invariant::IncomingArcContainer> _incomingArcs;

  // Map from VarID -> vector of InvariantID
  IdMap<VarIdBase, var::OutgoingArcContainer> _outgoingArcs;

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
  LocalId registerInvariantInput(InvariantId invariantId, VarIdBase inputId,
                                 bool isDynamic);

  /**
   * Register that source functionally defines varId
   * @param varId the variable that is defined by the invariant
   * @param invriant the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVar(VarIdBase varId, InvariantId invariant);

  void makeDynamicInputActive(Timestamp, InvariantId, LocalId);
  void makeDynamicInputInactive(Timestamp, InvariantId, LocalId);
  void makeAllDynamicInactive(Timestamp, VarIdBase);
  void commitOutgoingArcs(Timestamp, VarIdBase);

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
    return !_incomingArcs.at(id).incomingDynamic().empty();
  }

  [[nodiscard]] inline size_t numInputVars(InvariantId id) const {
    return _incomingArcs.at(id).numArcs();
  }

  [[nodiscard]] inline InvariantId definingInvariant(VarIdBase id) const {
    // Returns NULL_ID if id is a search variable (not defined by an invariant)
    return _definingInvariant.at(id);
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& varsDefinedBy(
      InvariantId invariantId) const {
    return _varsDefinedByInvariant.at(invariantId);
  }

  [[nodiscard]] inline const var::OutgoingArcContainer& outgoingArcs(
      VarId id) const {
    return _outgoingArcs.at(id.id);
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& staticInputVars(
      InvariantId invariantId) const {
    return _incomingArcs.at(invariantId).incomingStatic();
  }

  [[nodiscard]] inline const std::vector<invariant::IncomingDynamicArc>&
  dynamicInputVars(InvariantId invariantId) const {
    return _incomingArcs.at(invariantId).incomingDynamic();
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

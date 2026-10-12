#pragma once

#include <vector>

#include "atlantis/propagation/edges.hpp"
#include "atlantis/propagation/propagation/propagationQueue.hpp"
#include "atlantis/propagation/store/store.hpp"

namespace atlantis::propagation {

class PropagationGraph {
 public:
  struct LayerIndex {
    size_t layer;
    size_t index;
  };
  struct ListeningInvariantData {
    InvariantId invariantId;
    LocalId localId;
    ListeningInvariantData(const ListeningInvariantData& other) = default;
    ListeningInvariantData(InvariantId t_invariantId, LocalId t_localId);
    ListeningInvariantData& operator=(ListeningInvariantData&& other) noexcept;
  };

 private:
  std::vector<bool> _isEvaluationVar{};
  std::vector<bool> _isSearchVar{};
  std::vector<VarId> _searchVars{};
  std::vector<VarId> _evaluationVars{};

  const Store& _store;

  /**
   * Map from VarID -> InvariantId
   *
   * Maps to nullptr if not defined by any invariant.
   */
  std::vector<InvariantId> _definingInvariant;

  /**
   * Map from InvariantId -> list of VarViewId
   *
   * Maps an invariant to all variables it defines.
   */
  std::vector<std::vector<VarId>> _varsDefinedByInvariant;
  /**
   * Map from InvariantId -> list of VarViewId
   *
   * Maps an invariant to all its variable inputs.
   */
  // Given input variable with VarId x and invariant with InvariantId i, then
  // _staticInputVars[i] = <x, b>, where b = true iff x is a dynamic input
  // to i.
  std::vector<invariant::IncomingEdgeContainer> _incomingEdges;

  // Map from VarID -> vector of InvariantID
  std::vector<var::OutgoingEdgeContainer> _outgoingEdges;

  std::vector<std::vector<VarId>> _varsInLayer;
  std::vector<LayerIndex> _varLayerIndex;
  std::vector<size_t> _topologicalNumber;
  std::vector<bool> _layerHasDynamicCycle{};
  std::vector<size_t> _topologicalNumberOffset;
  bool _hasDynamicCycle{false};
  size_t _numInvariants{0};
  size_t _numVars{0};

  void partitionIntoLayers();
  void topologicallyOrder(Timestamp ts, size_t layer, bool updatePriorityQueue);
  void topologicallyOrder(Timestamp ts);

  struct PriorityCmp {
    PropagationGraph& graph;
    explicit PriorityCmp(PropagationGraph& g) : graph(g) {}
    bool operator()(VarId left, VarId right) const;
  };

  PropagationQueue _propagationQueue;

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
  void registerVar(VarId);

  /**
   * Register that inputId is an input of invariantId
   * @param invariantId the invariant
   * @param inputId the variable input
   * @param isDynamic true if the variable is a dynamic input to the
   * invariant.
   */
  LocalId registerInvariantInput(InvariantId invariantId, VarId inputId,
                                 bool isDynamic);

  /**
   * Register that source functionally defines varId
   * @param varId the variable that is defined by the invariant
   * @param invariantId the invariant defining the variable
   * @throw VarAlreadyDefinedException if the variable is already defined by an invariant.
   */
  void registerDefinedVar(VarId varId, InvariantId invariantId);

  void makeDynamicInputActive(Timestamp, InvariantId, LocalId);
  void makeAllDynamicInputsInactive(Timestamp, InvariantId);
  void makeDynamicInputInactive(Timestamp, InvariantId, LocalId);
  void makeAllDynamicInactive(Timestamp, VarId);
  void commitOutgoingEdges(Timestamp, VarId);

  /**
   * @brief topologically orders the layer
   *
   * @param ts
   * @param layer
   */
  void topologicallyOrder(Timestamp ts, size_t layer);

  [[nodiscard]] size_t numVars() const;

  [[nodiscard]] size_t numInvariants() const;

  [[nodiscard]] bool isEvaluationVar(VarId id) const;

  [[nodiscard]] bool isSearchVar(VarId id) const;

  [[nodiscard]] bool isDynamicInvariant(InvariantId invariantId) const;

  [[nodiscard]] inline size_t numInputVars(InvariantId invariantId) const {
    assert(invariantId < _incomingEdges.size());
    return _incomingEdges[invariantId].numEdges();
  }

  [[nodiscard]] InvariantId definingInvariant(VarId varId) const;

  [[nodiscard]] const std::vector<VarId>& varsDefinedBy(
      InvariantId invariantId) const;

  [[nodiscard]] const var::OutgoingEdgeContainer& outgoingEdges(
      VarId varId) const;

  [[nodiscard]] const std::vector<VarId>& staticInputVars(
      InvariantId invariantId) const;
  [[nodiscard]] VarId dynamicInputVar(Timestamp ts, InvariantId invariantId) const noexcept;

  [[nodiscard]] const std::vector<invariant::IncomingDynamicEdge>&
  dynamicInputVars(InvariantId invariantId) const;

  [[nodiscard]] const std::vector<VarId>& searchVars() const;

  [[nodiscard]] const std::vector<VarId>& evaluationVars() const;

  void clearPropagationQueue();

  [[nodiscard]] bool propagationQueueEmpty() const;

  [[nodiscard]] VarId dequeuePropagationQueue();

  [[nodiscard]] bool hasDynamicCycle() const noexcept;

  [[nodiscard]] bool hasDynamicCycle(size_t layer) const;

  [[nodiscard]] size_t numLayers() const noexcept;

  [[nodiscard]] size_t numVarsInLayer(size_t layer) const noexcept;

  [[nodiscard]] const std::vector<VarId>& varsInLayer(
      size_t layer) const noexcept;

  [[nodiscard]] size_t varLayer(VarId id) const;

  [[nodiscard]] size_t invariantLayer(InvariantId invariantId) const;

  [[nodiscard]] size_t varPosition(VarId id) const;

  [[nodiscard]] size_t invariantPosition(InvariantId invariantId) const;

  void enqueuePropagationQueue(VarId id);

  [[nodiscard]] const invariant::IncomingEdgeContainer& incomingEdges(InvariantId invId) const;
};

}  // namespace atlantis::propagation

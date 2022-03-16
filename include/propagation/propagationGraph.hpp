#pragma once

#include <vector>

#include "core/types.hpp"
#include "layeredPropagationQueue.hpp"
#include "propagationQueue.hpp"
#include "utils/idMap.hpp"

class PropagationGraph {
 protected:
  size_t _numInvariants;
  size_t _numVariables;

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
  IdMap<InvariantId, std::vector<VarIdBase>> _inputVariables;

  // Map from VarId -> vector of InvariantId
  IdMap<VarIdBase, std::vector<InvariantId>> _listeningInvariants;

  std::vector<bool> _isEvaluationVariable;
  std::vector<bool> _isSearchVariable;

  std::vector<VarIdBase> _searchVariables;
  std::vector<VarIdBase> _evaluationVariables;

  struct Topology {
    std::vector<size_t> variablePosition;
    std::vector<size_t> invariantPosition;

    PropagationGraph& graph;
    Topology() = delete;
    explicit Topology(PropagationGraph& g) : graph(g) {}
    void computeNoCycles();
    void computeWithCycles();
    void computeLayersWithCycles();
    void computeInvariantFromVariables();
    inline size_t position(VarIdBase id) { return variablePosition[id.id]; }
    inline size_t position(InvariantId invariantId) {
      assert(invariantId < invariantPosition.size());
      return invariantPosition[invariantId];
    }
  } _topology;

  struct PriorityCmp {
    PropagationGraph& graph;
    explicit PriorityCmp(PropagationGraph& g) : graph(g) {}
    bool operator()(VarIdBase left, VarIdBase right) {
      return graph._topology.position(left) > graph._topology.position(right);
    }
  };

  PropagationQueue _propagationQueue;

 public:
  PropagationGraph() : PropagationGraph(1000) {}
  explicit PropagationGraph(size_t expectedSize);

  /**
   * update internal datastructures based on currently registered  variables and
   * invariants.
   */
  void close();

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
   */
  void registerInvariantInput(InvariantId invariantId, VarIdBase inputId);

  /**
   * Register that source functionally defines varId
   * @param varId the variable that is defined by the invariant
   * @param invriant the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarIdBase varId, InvariantId invariant);

  [[nodiscard]] inline size_t numVariables() const {
    return _numVariables;  // this ignores null var
  }

  [[nodiscard]] inline size_t numInvariants() const {
    return _numInvariants;  // this ignores null invariant
  }

  inline bool isEvaluationVar(VarIdBase id) {
    assert(id < _isEvaluationVariable.size());
    return _isEvaluationVariable[id];
  }

  inline bool isSearchVariable(VarIdBase id) {
    assert(id < _isSearchVariable.size());
    return _isSearchVariable.at(id);
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

  [[nodiscard]] inline const std::vector<VarIdBase>& inputVariables(
      InvariantId invariantId) const {
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

  inline void enqueuePropagationQueue(VarIdBase id) {
    _propagationQueue.push(id);
  }
};

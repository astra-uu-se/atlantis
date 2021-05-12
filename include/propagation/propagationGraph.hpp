#pragma once

#include <vector>

#include "core/types.hpp"
#include "layeredPropagationQueue.hpp"
#include "propagationQueue.hpp"
#include "utils/idMap.hpp"

class PropagationGraph {
 protected:
  size_t m_numInvariants;
  size_t m_numVariables;

  /**
   * Map from VarID -> InvariantId
   *
   * Maps to nullptr if not defined by any invariant.
   */
  IdMap<VarIdBase, InvariantId> m_definingInvariant;

  /**
   * Map from InvariantId -> list of VarId
   *
   * Maps an invariant to all variables it defines.
   */
  IdMap<InvariantId, std::vector<VarIdBase>> m_variablesDefinedByInvariant;
  /**
   * Map from InvariantId -> list of VarId
   *
   * Maps an invariant to all variables it depends on (its inputs).
   */
  IdMap<InvariantId, std::vector<VarIdBase>> m_inputVariables;

  // Map from VarId -> vector of InvariantId
  IdMap<VarIdBase, std::vector<InvariantId>> m_listeningInvariants;

  std::vector<bool> m_isOutputVar;
  std::vector<bool> m_isDecisionVar;

  struct Topology {
    std::vector<size_t> m_variablePosition;
    std::vector<size_t> m_invariantPosition;

    PropagationGraph& graph;
    Topology() = delete;
    explicit Topology(PropagationGraph& g) : graph(g) {}
    void computeNoCycles();
    void computeWithCycles();
    void computeLayersWithCycles();
    void computeInvariantFromVariables();
    inline size_t getPosition(VarIdBase id) {
      return m_variablePosition[id.id];
    }
    inline size_t getPosition(InvariantId id) {
      return m_invariantPosition.at(id);
    }
  } m_topology;

  friend class PropagationEngine;

  struct PriorityCmp {
    PropagationGraph& graph;
    explicit PriorityCmp(PropagationGraph& g) : graph(g) {}
    bool operator()(VarIdBase left, VarIdBase right) {
      return graph.m_topology.getPosition(left) >
             graph.m_topology.getPosition(right);
    }
  };

  PropagationQueue m_propagationQueue;
  // LayeredPropagationQueue m_propagationQueue;
  // std::priority_queue<VarIdBase, std::vector<VarIdBase>,
  //                     PropagationGraph::PriorityCmp>
  //     m_propagationQueue;

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
   * Register that Invariant to depends on variable from depends on dependency
   * @param depends the invariant that the variable depends on
   * @param source the depending variable
   */
  void registerInvariantDependsOnVar(InvariantId depends, VarIdBase source);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param depends the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarIdBase depends, InvariantId source);

  [[nodiscard]] inline size_t getNumVariables() const {
    return m_numVariables;  // this ignores null var
  }

  [[nodiscard]] inline size_t getNumInvariants() const {
    return m_numInvariants;  // this ignores null invariant
  }

  inline bool isOutputVar(VarIdBase id) { return m_isOutputVar.at(id); }

  inline bool isDecisionVar(VarIdBase id) { return m_isDecisionVar.at(id); }

  inline InvariantId getDefiningInvariant(VarIdBase v) {
    // Returns NULL_ID is not defined.
    return m_definingInvariant.at(v);
  }

  [[nodiscard]] inline const std::vector<VarIdBase>& getVariablesDefinedBy(
      InvariantId inv) const {
    return m_variablesDefinedByInvariant.at(inv);
  }
};

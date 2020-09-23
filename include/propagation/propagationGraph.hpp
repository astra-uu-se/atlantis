#pragma once

#include <assert.h>

#include <memory>
#include <queue>
#include <vector>

#include "../core/types.hpp"

class PropagationGraph {
 protected:
  size_t m_numInvariants;
  size_t m_numVariables;

  /**
   * Map from VarID -> InvariantId
   *
   * Maps to nullptr if not defined by any invariant.
   */
  std::vector<InvariantId> m_definingInvariant;

  /**
   * Map from InvariantId -> list of VarID
   *
   * Maps an invariant to all variables it defines.
   */
  std::vector<std::vector<VarId>> m_variablesDefinedByInvariant;
  /**
   * Map from InvariantId -> list of VarID
   *
   * Maps an invariant to all variables it depends on (its inputs).
   */
  std::vector<std::vector<VarId>> m_inputVariables;

  // Map from VarID -> vector of InvariantID
  std::vector<std::vector<InvariantId>> m_listeningInvariants;

  std::vector<bool> m_isOutputVar;
  std::vector<bool> m_isInputVar;

  struct Topology {
    std::vector<size_t> m_variablePosition;
    std::vector<size_t> m_invariantPosition;

    PropagationGraph& graph;
    Topology() = delete;
    Topology(PropagationGraph& g) : graph(g) {}
    void computeNoCycles();
    void computeWithCycles();
    void computeInvariantFromVariables();
    inline size_t getPosition(VarId id) { return m_variablePosition.at(id); }
    inline size_t getPosition(InvariantId id) {
      return m_invariantPosition.at(id);
    }
  } m_topology;

  friend class PropagationEngine;

  struct PriorityCmp {
    PropagationGraph& graph;
    PriorityCmp(PropagationGraph& g) : graph(g) {}
    bool operator()(VarId left, VarId right) {
      return graph.m_topology.getPosition(left) <
             graph.m_topology.getPosition(right);
    }
  };

 public:
  PropagationGraph() : PropagationGraph(1000) {}
  PropagationGraph(size_t expectedSize);

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
  void registerVar(VarId);

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param depends the invariant that the variable depends on
   * @param source the depending variable
   */
  void registerInvariantDependsOnVar(InvariantId depends, VarId source);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param depends the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId depends, InvariantId source);

  inline size_t getNumVariables() {
    return m_numVariables;  // this ignores null var
  }

  inline size_t getNumInvariants() {
    return m_numInvariants;  // this ignores null invariant
  }

  inline bool isOutputVar(VarId id) { return m_isOutputVar.at(id); }

  inline bool isInputVar(VarId id) { return m_isInputVar.at(id); }

  inline InvariantId getDefiningInvariant(VarId v) {
    // Returns NULL_ID is not defined.
    return m_definingInvariant.at(v);
  }

  inline const std::vector<VarId>& getVariablesDefinedBy(
      InvariantId inv) const {
    return m_variablesDefinedByInvariant.at(inv);
  }
};

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

  struct InvariantDependencyData {
    InvariantId id;
    LocalId localId;
    Int data;
  };
  // Map from VarID -> vector of InvariantID
  std::vector<std::vector<InvariantDependencyData>> m_listeningInvariants;

  // The last time when an intVar was commited to
  std::vector<Timestamp> m_varsLastCommit;

  std::queue<VarId> m_modifiedVariables;

  struct Topology {
    std::vector<size_t> m_variablePosition;
    std::vector<size_t> m_invariantPosition;

    PropagationGraph& graph;
    Topology(PropagationGraph& g) : graph(g) {}
    void computeTopologyNoCycles();
    void computeTopologyWithCycles();
    void computeTopologyBundleCycles();
    size_t getPosition(VarId id) { return m_variablePosition.at(id); }
    size_t getPosition(InvariantId id) { return m_invariantPosition.at(id); }
  } m_topology;

 public:
  PropagationGraph() : PropagationGraph(1000) {}
  PropagationGraph(size_t expectedSize);

  void notifyMaybeChanged(const Timestamp& t, VarId id);

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
   * @param localId the id of the depending variable in the invariant
   * @param data additional data
   */
  void registerInvariantDependsOnVar(InvariantId depends, VarId source,
                                     LocalId localId, Int data);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param depends the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId depends, InvariantId source);

  void close() {
    // m_topology.computeTopologyNoCycles();
    m_topology.computeTopologyWithCycles();
    }

  size_t getNumVariables() {
    return m_numVariables;  //this ignores null var
  }

  size_t getNumInvariants() {
    return m_numInvariants;  //this ignores null invariant
  }

  size_t getTopologicalKey(VarId id){
    return m_topology.m_variablePosition.at(id);
  }

  size_t getTopologicalKey(InvariantId id) {
    return m_topology.m_invariantPosition.at(id);
  }
};

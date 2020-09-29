#pragma once

#include <queue>

#include "propagation/propagationGraph.hpp"

class Engine;  // forward declare

class TopDownPropagationGraph : public PropagationGraph {
 private:
  // The last time that a variable was notified as changed.
  std::vector<Timestamp> m_varsLastChange;

  struct Topology {
    std::vector<size_t> m_variablePosition;
    std::vector<size_t> m_invariantPosition;

    TopDownPropagationGraph& graph;
    Topology() = delete;
    Topology(TopDownPropagationGraph& g) : graph(g) {}
    void computeNoCycles();
    void computeNoCyclesException();
    void computeWithCycles();
    void computeBundleCycles();
    void computeInvariantFromVariables();
    size_t getPosition(VarId id) {
      assert(id.idType == VarIdType::var);
      return m_variablePosition.at(id);
    }
    size_t getPosition(InvariantId id) { return m_invariantPosition.at(id); }
  } m_topology;

  struct PriorityCmp {
    Topology& topology;
    PriorityCmp(Topology& t) : topology(t) {}

    bool operator()(VarId left, VarId right) {
      assert(left.idType == VarIdType::var);
      assert(right.idType == VarIdType::var);
      return topology.getPosition(left) < topology.getPosition(right);
    }
  };

 protected:
  std::priority_queue<VarId, std::vector<VarId>, PriorityCmp>
      m_modifiedVariables;

 public:
  TopDownPropagationGraph() : TopDownPropagationGraph(1000) {}
  TopDownPropagationGraph(size_t expectedSize);

  virtual void close() override {
    PropagationGraph::close();
    // m_topology.computeNoCycles();
    // m_topology.computeNoCyclesException();
    // m_topology.computeWithCycles();
    m_topology.computeBundleCycles();
  }

  virtual void notifyMaybeChanged(Timestamp, VarId id) override;

  virtual VarId getNextStableVariable(Timestamp) override;
  virtual void registerVar(VarId) override;

  size_t getTopologicalKey(VarId id) {
    assert(id.idType == VarIdType::var);
    return m_topology.m_variablePosition.at(id);
  }

  size_t getTopologicalKey(InvariantId id) {
    return m_topology.m_invariantPosition.at(id);
  }
};

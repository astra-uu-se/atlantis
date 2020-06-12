#pragma once

#include <queue>

#include "propagation/propagationGraph.hpp"

class TopDownPropagationGraph : public PropagationGraph {
 private:
  // The last time when an intVar was modified to
  std::vector<Timestamp> m_varsLastChange;

  struct PriorityCmp {
    Topology& topology;
    PriorityCmp(Topology& t) : topology(t) {}

    bool operator()(VarId left, VarId right) {
      return topology.getPosition(left) < topology.getPosition(right);
    }
  };

  std::priority_queue<VarId, std::vector<VarId>, PriorityCmp>
      m_modifiedVariables;

 public:
  TopDownPropagationGraph() : TopDownPropagationGraph(1000) {}
  TopDownPropagationGraph(size_t expectedSize);

  void notifyMaybeChanged(const Timestamp& t, VarId id) override;

  /**
   * Register a variable in the propagation graph.
   */
  virtual void registerVar(VarId) override;
};

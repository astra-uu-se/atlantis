#pragma once

#include <queue>

#include "propagation/propagationGraph.hpp"

class Engine;  // forward declare

class TopDownPropagationGraph : public PropagationGraph {
 private:
  // The last time that a variable was notified as changed.
  std::vector<Timestamp> m_varsLastChange;

  struct PriorityCmp {
    Topology& topology;
    PriorityCmp(Topology& t) : topology(t) {}

    bool operator()(VarId left, VarId right) {
      return topology.getPosition(left) < topology.getPosition(right);
    }
  };

 protected:
  std::priority_queue<VarId, std::vector<VarId>, PriorityCmp>
      m_modifiedVariables;

 public:
  TopDownPropagationGraph() : TopDownPropagationGraph(1000) {}
  TopDownPropagationGraph(size_t expectedSize);

  virtual void notifyMaybeChanged(const Timestamp&, VarId id) override;

  virtual VarId getNextStableVariable(const Timestamp&) override;
  virtual void registerVar(VarId) override;
};

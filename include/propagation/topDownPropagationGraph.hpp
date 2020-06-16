#pragma once

#include <queue>

#include "propagation/propagationGraph.hpp"

class Engine;  // forward declare

class TopDownPropagationGraph : public PropagationGraph {
 private:
  // The last time that a variable was notified as changed.
  std::vector<Timestamp> m_varsLastChange;

  // The last time when a variable was propagated (i.e., returned by the
  // function getNextStableVariable)
  std::vector<Timestamp> m_propagatedAt;

  // Shared tmp container for storing a list of variables.  Although this
  // should be local to each function, there is no need to reallocate all that
  // space each call.
  std::vector<VarId> m_tmpVarContainer;
  std::vector<bool> m_tmpMarkContainer;

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

  void notifyMaybeChanged(const Timestamp&, VarId id) override;

  void schedulePropagationFor(const Timestamp&, const Engine&, VarId);

  virtual VarId getNextStableVariable(const Timestamp&) override;
  virtual void registerVar(VarId) override;

  // virtual void registerInvariant(InvariantId) override;
};

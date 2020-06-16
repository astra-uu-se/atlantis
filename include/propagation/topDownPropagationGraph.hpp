#pragma once

#include <queue>

#include "propagation/propagationGraph.hpp"

class Engine;  // forward declare

class TopDownPropagationGraph : public PropagationGraph {
 private:
  // The last time that a variable was notified as changed.
  std::vector<Timestamp> m_varsLastChange;

  // The timestamp where var/invariant was last known to be marked.
  std::vector<Timestamp> m_invariantMark;
  std::vector<Timestamp> m_varMark;

  // The last time when a variable was propagated (i.e., returned by the
  // function getNextStableVariable)
  std::vector<Timestamp> m_propagatedAt;

  std::vector<VarId> m_variablesToExplore;

  // Shared tmp container for storing a list of variables.  Although this
  // should be local to each function, there is no need to reallocate all that
  // space each call.
  std::vector<VarId> m_tmpVarContainer;
  std::vector<bool> m_tmpBoolContainer;

  struct PriorityCmp {
    Topology& topology;
    PriorityCmp(Topology& t) : topology(t) {}

    bool operator()(VarId left, VarId right) {
      return topology.getPosition(left) < topology.getPosition(right);
    }
  };

  std::priority_queue<VarId, std::vector<VarId>, PriorityCmp>
      m_modifiedVariables;

 protected:
  friend class Engine;
  void clearForPropagation();
  void registerForPropagation(const Timestamp&, VarId);
  void schedulePropagation(const Timestamp&, const Engine&);

 public:
  TopDownPropagationGraph() : TopDownPropagationGraph(1000) {}
  TopDownPropagationGraph(size_t expectedSize);

  void notifyMaybeChanged(const Timestamp&, VarId id) override;

  virtual VarId getNextStableVariable(const Timestamp&) override;
  virtual void registerVar(VarId) override;

  virtual bool isActive(const Timestamp&, VarId) override;
  virtual bool isActive(const Timestamp&, InvariantId) override;

  virtual void registerInvariant(InvariantId) override;
};

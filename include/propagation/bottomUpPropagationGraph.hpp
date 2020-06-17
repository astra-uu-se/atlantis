#pragma once

#include <queue>

#include "propagation/propagationGraph.hpp"

class BottomUpPropagationGraph : public PropagationGraph {
 private:
 
 public:
  BottomUpPropagationGraph() : BottomUpPropagationGraph(1000) {}
  BottomUpPropagationGraph(size_t expectedSize);

  void notifyMaybeChanged(const Timestamp& t, VarId id) override;

  // virtual VarId getNextStableVariable(const Timestamp& t) override;

  /**
   * Register a variable in the propagation graph.
   */
  virtual void registerVar(VarId) override;
};

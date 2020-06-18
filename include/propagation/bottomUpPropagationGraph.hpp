#pragma once

#include <queue>

#include "core/intVar.hpp"
#include "propagation/propagationGraph.hpp"
#include "core/engine.hpp"
// class Engine;  // forward declare

class BottomUpPropagationGraph : public PropagationGraph {
 private:
  std::vector<VarId> variableStack;
  std::vector<InvariantId> invariantStack;
  std::vector<bool> isUpToDate;
  std::vector<bool> hasVisited;

  inline void setVisited(VarId v) { hasVisited.at(v) = true; }
  inline void fixpoint(VarId v) { isUpToDate.at(v) = true; }
  // We expand an invariant by pushing it and its first input variable onto each
  // stack.
  inline void expandInvariant(InvariantId inv) {
    VarId nextVar =
        m_engine.getStore().getInvariant(inv).getNextDependency(m_engine.getCurrentTime());
    assert(nextVar.id !=
           NULL_ID);  // Invariant must have at least one dependency, and this
                      // should be the first (and only) time we expand it
    variableStack.push_back(nextVar);
    invariantStack.push_back(inv);
  }

  inline void notifyCurrentInvariant(VarId id) {
    IntVar variable = m_engine.getStore().getConstIntVar(id);
    m_engine.getStore()
        .getInvariant(invariantStack.back())
        .notifyCurrentDependencyChanged(m_engine.getCurrentTime(),
                                        variable.getCommittedValue(),
                                        variable.getValue(m_engine.getCurrentTime()));
  }

  inline void nextVar() {
    variableStack.pop_back();
    VarId nextVar = m_engine.getStore()
                        .getInvariant(invariantStack.back())
                        .getNextDependency(m_engine.getCurrentTime());
    if (nextVar.id == NULL_ID) {
      // The invariant has finished propagating, so all defined vars can be
      // marked as up to date.
      // Do this with member array of timestamps.
      for (auto defVar : variableDefinedBy(invariantStack.back())) {
        fixpoint(defVar);
      }
      invariantStack.pop_back();
    } else {
      variableStack.push_back(nextVar);
    }
  }
  Engine& m_engine;

 public:
  // BottomUpPropagationGraph() : BottomUpPropagationGraph(1000) {}
  BottomUpPropagationGraph(Engine& e, size_t expectedSize);

  void propagate();
  void clearForQuery();
  void registerForQuery(VarId);

  virtual void notifyMaybeChanged(const Timestamp& t, VarId id) override;

  // virtual VarId getNextStableVariable(const Timestamp& t) override;

  /**
   * Register a variable in the propagation graph.
   */
  virtual void registerVar(VarId) override;
  virtual void registerInvariant(InvariantId) override;
};

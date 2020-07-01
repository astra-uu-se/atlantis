#pragma once

#include <queue>

// #include "core/engine.hpp"
#include "core/intVar.hpp"
#include "propagation/propagationGraph.hpp"
#include "exceptions/exceptions.hpp"
class Engine;  // forward declare

class BottomUpPropagationGraph : public PropagationGraph {
 private:
  std::vector<VarId> variableStack_;
  size_t varStackIdx_ = 0;
  std::vector<InvariantId> invariantStack_;
  size_t invariantStackIdx_ = 0;
  std::vector<Timestamp> varStableAt;  // last timestamp when a VarID was stable
                                    // (i.e., will not change)
  std::vector<Timestamp> invariantStableAt;
  std::vector<bool> varIsOnStack;
  std::vector<bool> invariantIsOnStack;

  void pushVariableStack(VarId v);
  void popVariableStack();
  VarId peekVariableStack();
  void pushInvariantStack(InvariantId v);
  void popInvariantStack();
  InvariantId peekInvariantStack();
  void markStable(Timestamp t, VarId v);
  void markStable(Timestamp t, InvariantId v);
  bool isStable(Timestamp t, VarId v);
  bool isStable(Timestamp t, InvariantId v);

  // We expand an invariant by pushing it and its first input variable onto each
  // stack.
  void expandInvariant(InvariantId inv);
  void notifyCurrentInvariant();
  bool visitNextVariable();

  void commitAndPostpone(Timestamp, VarId);

  Engine& m_engine;

 public:
  // BottomUpPropagationGraph() : BottomUpPropagationGraph(1000) {}
  BottomUpPropagationGraph(Engine& e, size_t expectedSize);

  void fullPropagateAndCommit(Timestamp);

  void propagate(Timestamp);

  void propagate2(Timestamp);
  void lazyPropagateAndCommit(Timestamp);

  void clearForPropagation();
  /**
   * Register than we want to compute the value of v at time t
   */
  void registerForPropagation(Timestamp t, VarId v);

  virtual void notifyMaybeChanged(Timestamp t, VarId id) override;

  [[nodiscard]] virtual VarId getNextStableVariable(Timestamp) override {
    assert(false);
    return NULL_ID;
  }

  /**
   * Register a variable in the propagation graph.
   */
  virtual void registerVar(VarId) override;
  virtual void registerInvariant(InvariantId) override;
};

inline void BottomUpPropagationGraph::pushVariableStack(VarId v) {
  varIsOnStack.at(v) = true;
  variableStack_[varStackIdx_++] = v;
}
inline void BottomUpPropagationGraph::popVariableStack() {
  varIsOnStack.at(variableStack_[--varStackIdx_]) = false;
}
inline VarId BottomUpPropagationGraph::peekVariableStack(){
  return variableStack_[varStackIdx_-1];
}

inline void BottomUpPropagationGraph::pushInvariantStack(InvariantId i) {
  if (invariantIsOnStack.at(i)) {
    throw DynamicCycleException();
  }
  invariantIsOnStack.at(i) = true;
  invariantStack_[invariantStackIdx_++] = i;
}
inline void BottomUpPropagationGraph::popInvariantStack() {
  invariantIsOnStack.at(invariantStack_[--invariantStackIdx_]) = false;
}

inline InvariantId BottomUpPropagationGraph::peekInvariantStack(){
  return invariantStack_[invariantStackIdx_-1];
}

inline void BottomUpPropagationGraph::markStable(Timestamp t, VarId v) {
  varStableAt.at(v) = t;
}

inline bool BottomUpPropagationGraph::isStable(Timestamp t, VarId v) {
  return varStableAt.at(v) == t;
}

inline void BottomUpPropagationGraph::markStable(Timestamp t, InvariantId v) {
  invariantStableAt.at(v) = t;
}

inline bool BottomUpPropagationGraph::isStable(Timestamp t, InvariantId v) {
  return invariantStableAt.at(v) == t;
}



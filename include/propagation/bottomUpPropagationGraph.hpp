#pragma once

#include <queue>

// #include "core/engine.hpp"
#include "core/intVar.hpp"
#include "propagation/propagationGraph.hpp"
class Engine;  // forward declare

class BottomUpPropagationGraph : public PropagationGraph {
 private:
  std::vector<VarId> variableStack;
  std::vector<InvariantId> invariantStack;
  std::vector<Timestamp> stableAt;  // last timestamp when a VarID was stable
                                    // (i.e., will not change)
  std::vector<bool> varIsOnStack;
  std::vector<bool> invariantIsOnStack;

  void pushVariableStack(VarId v);
  void popVariableStack();
  void pushInvariantStack(InvariantId v);
  void popInvariantStack();
  void markStable(Timestamp t, VarId v);

  bool isStable(Timestamp t, VarId v);

  // We expand an invariant by pushing it and its first input variable onto each
  // stack.
  void expandInvariant(InvariantId inv);
  void notifyCurrentInvariant();
  bool visitNextVariable();

  void printVarStack() {
    std::cout << "Variable stack: [";
    for (auto id : variableStack) {
      std::cout << id << ", ";
    }
    std::cout << "]\n";
  }

  void printInvariantStack() {
    std::cout << "Invariant stack: [";
    for (auto id : invariantStack) {
      std::cout << id << ", ";
    }
    std::cout << "]\n";
  }

  Engine& m_engine;

 public:
  // BottomUpPropagationGraph() : BottomUpPropagationGraph(1000) {}
  BottomUpPropagationGraph(Engine& e, size_t expectedSize);

  void fullPropagateAndCommit(Timestamp);

  void propagate(Timestamp);
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

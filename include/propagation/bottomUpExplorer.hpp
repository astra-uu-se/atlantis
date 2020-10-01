#pragma once

#include <assert.h>

#include <memory>
#include <vector>

#include "core/tracer.hpp"
#include "core/types.hpp"
#include "exceptions/exceptions.hpp"

<<<<<<< HEAD:include/propagation/bottomUpPropagationGraph.hpp
class BottomUpPropagationGraph : public PropagationGraph {
 private:
  Engine& m_engine;
=======
// #include "core/propagationEngine.hpp"

// Forward declare PropagationEngine
class PropagationEngine;

class BottomUpExplorer {
  PropagationEngine& m_engine;

  std::vector<VarId> variableStack_;
>>>>>>> develop:include/propagation/bottomUpExplorer.hpp
  size_t varStackIdx_ = 0;
  size_t invariantStackIdx_ = 0;
<<<<<<< HEAD:include/propagation/bottomUpPropagationGraph.hpp
  std::vector<VarId> variableStack_;
  std::vector<InvariantId> invariantStack_;
  std::vector<Timestamp> varStableAt;  // last timestamp when a VarID was stable
                                       // (i.e., will not change)
=======
  std::vector<Timestamp> varStableAt;  // last timestamp when a VarID was
                                       // stable (i.e., will not change)
>>>>>>> develop:include/propagation/bottomUpExplorer.hpp
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

  // We expand an invariant by pushing it and its first input variable onto
  // each stack.
  void expandInvariant(InvariantId inv);
  void notifyCurrentInvariant();
  bool visitNextVariable();

  void commitAndPostpone(Timestamp, VarId);

<<<<<<< HEAD:include/propagation/bottomUpPropagationGraph.hpp
  

 public:
  // BottomUpPropagationGraph() : BottomUpPropagationGraph(1000) {}
  BottomUpPropagationGraph(Engine& e, size_t expectedSize);

  void fullPropagateAndCommit(Timestamp);

  void propagate(Timestamp);

  void propagate2(Timestamp);
  void lazyPropagateAndCommit(Timestamp);
=======
public: 
  BottomUpExplorer() = delete;
  BottomUpExplorer(PropagationEngine& e, size_t expectedSize);
>>>>>>> develop:include/propagation/bottomUpExplorer.hpp

  void registerVar(VarId);
  void registerInvariant(InvariantId);
  /**
   * Register than we want to compute the value of v at time t
   */
  void registerForPropagation(Timestamp t, VarId v);

  void clearRegisteredVariables();

  void propagate(Timestamp currentTime);
};

<<<<<<< HEAD:include/propagation/bottomUpPropagationGraph.hpp
inline void BottomUpPropagationGraph::pushVariableStack(VarId v) {
  assert(v.idType == VarIdType::var);
=======
inline void BottomUpExplorer::registerForPropagation(Timestamp, VarId id) {
  // TODO: why not set varIsOnStack.at(v) = true;?
  // I remember that there was some technical reason but this need to be
  // documented. Note that this might overflow the stack otherwise.
  variableStack_[varStackIdx_++] = id;
}

inline void BottomUpExplorer::clearRegisteredVariables() { varStackIdx_ = 0; }

inline void BottomUpExplorer::pushVariableStack(VarId v) {
>>>>>>> develop:include/propagation/bottomUpExplorer.hpp
  varIsOnStack.at(v) = true;
  variableStack_[varStackIdx_++] = v;
}
inline void BottomUpExplorer::popVariableStack() {
  varIsOnStack.at(variableStack_[--varStackIdx_]) = false;
}
inline VarId BottomUpExplorer::peekVariableStack() {
  return variableStack_[varStackIdx_ - 1];
}

inline void BottomUpExplorer::pushInvariantStack(InvariantId i) {
  if (invariantIsOnStack.at(i)) {
    throw DynamicCycleException();
  }
  invariantIsOnStack.at(i) = true;
  invariantStack_[invariantStackIdx_++] = i;
}
inline void BottomUpExplorer::popInvariantStack() {
  invariantIsOnStack.at(invariantStack_[--invariantStackIdx_]) = false;
}

inline InvariantId BottomUpExplorer::peekInvariantStack() {
  return invariantStack_[invariantStackIdx_ - 1];
}

<<<<<<< HEAD:include/propagation/bottomUpPropagationGraph.hpp
inline void BottomUpPropagationGraph::markStable(Timestamp t, VarId v) {
  assert(v.idType == VarIdType::var);
  varStableAt.at(v) = t;
}

inline bool BottomUpPropagationGraph::isStable(Timestamp t, VarId v) {
  assert(v.idType == VarIdType::var);
=======
inline void BottomUpExplorer::markStable(Timestamp t, VarId v) {
  varStableAt.at(v) = t;
}

inline bool BottomUpExplorer::isStable(Timestamp t, VarId v) {
>>>>>>> develop:include/propagation/bottomUpExplorer.hpp
  return varStableAt.at(v) == t;
}

inline void BottomUpExplorer::markStable(Timestamp t, InvariantId v) {
  invariantStableAt.at(v) = t;
}

inline bool BottomUpExplorer::isStable(Timestamp t, InvariantId v) {
  return invariantStableAt.at(v) == t;
}
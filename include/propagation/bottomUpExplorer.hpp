#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "utils/idMap.hpp"

// Forward declare PropagationEngine
class PropagationEngine;

class BottomUpExplorer {
  PropagationEngine& m_engine;

  std::vector<VarId> variableStack_;
  size_t varStackIdx_ = 0;
  std::vector<InvariantId> invariantStack_;
  size_t invariantStackIdx_ = 0;
  IdMap<VarId, Timestamp> varStableAt;  // last timestamp when a VarID was
                                        // stable (i.e., will not change)
  IdMap<InvariantId, Timestamp> invariantStableAt;
  IdMap<VarId, bool> varIsOnStack;
  IdMap<InvariantId, bool> invariantIsOnStack;

  void pushVariableStack(VarId v);
  void popVariableStack();
  VarId peekVariableStack();
  void pushInvariantStack(InvariantId);
  void popInvariantStack();
  InvariantId peekInvariantStack();
  void markStable(Timestamp t, VarId v);
  bool isStable(Timestamp t, VarId v);
  bool isStable(Timestamp t, InvariantId v);

  // We expand an invariant by pushing it and its first input variable onto
  // each stack.
  void expandInvariant(InvariantId inv);
  void notifyCurrentInvariant();
  bool visitNextVariable();

 public:
  BottomUpExplorer() = delete;
  BottomUpExplorer(PropagationEngine& e, size_t expectedSize);

  void registerVar(VarId);
  void registerInvariant(InvariantId);
  /**
   * Register than we want to compute the value of v at time t
   */
  void registerForPropagation(Timestamp, VarId);

  void clearRegisteredVariables();

  template <bool DoCommit>
  void propagate(Timestamp currentTime);
};

inline void BottomUpExplorer::registerForPropagation(Timestamp, VarId id) {
  // TODO: why not set varIsOnStack.at(v) = true;?
  // I remember that there was some technical reason but this need to be
  // documented. Note that this might overflow the stack otherwise.
  variableStack_[varStackIdx_++] = id;
}

inline void BottomUpExplorer::clearRegisteredVariables() { varStackIdx_ = 0; }

inline void BottomUpExplorer::pushVariableStack(VarId v) {
  varIsOnStack.set(v, true);
  variableStack_[varStackIdx_++] = v;
}
inline void BottomUpExplorer::popVariableStack() {
  varIsOnStack.set(variableStack_[--varStackIdx_], false);
}
inline VarId BottomUpExplorer::peekVariableStack() {
  return variableStack_[varStackIdx_ - 1];
}

inline void BottomUpExplorer::pushInvariantStack(InvariantId i) {
  if (invariantIsOnStack.get(i)) {
    throw DynamicCycleException();
  }
  invariantIsOnStack.set(i, true);
  invariantStack_[invariantStackIdx_++] = i;
}
inline void BottomUpExplorer::popInvariantStack() {
  invariantIsOnStack.set(invariantStack_[--invariantStackIdx_], false);
}

inline InvariantId BottomUpExplorer::peekInvariantStack() {
  return invariantStack_[invariantStackIdx_ - 1];
}

inline void BottomUpExplorer::markStable(Timestamp t, VarId v) {
  varStableAt[v] = t;
}

inline bool BottomUpExplorer::isStable(Timestamp t, VarId v) {
  return varStableAt.at(v) == t;
}

inline bool BottomUpExplorer::isStable(Timestamp t, InvariantId v) {
  return invariantStableAt.at(v) == t;
}

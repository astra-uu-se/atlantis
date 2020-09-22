#pragma once

#include <assert.h>

#include <memory>
#include <vector>

#include "core/tracer.hpp"
#include "core/types.hpp"
#include "exceptions/exceptions.hpp"

// #include "core/propagationEngine.hpp"

// Forward declare PropagationEngine
class PropagationEngine;

class BottomUpExplorer {
  PropagationEngine& m_engine;

  std::vector<VarId> variableStack_;
  size_t varStackIdx_ = 0;
  std::vector<InvariantId> invariantStack_;
  size_t invariantStackIdx_ = 0;
  std::vector<Timestamp> varStableAt;  // last timestamp when a VarID was
                                       // stable (i.e., will not change)
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

public: 
  BottomUpExplorer() = delete;
  BottomUpExplorer(PropagationEngine& e):m_engine(e){}

  void registerVar(VarId);
  void registerInvariant(InvariantId);
};


inline void BottomUpExplorer::pushVariableStack(VarId v) {
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

inline void BottomUpExplorer::markStable(Timestamp t, VarId v) {
  varStableAt.at(v) = t;
}

inline bool BottomUpExplorer::isStable(Timestamp t, VarId v) {
  return varStableAt.at(v) == t;
}

inline void BottomUpExplorer::markStable(Timestamp t, InvariantId v) {
  invariantStableAt.at(v) = t;
}

inline bool BottomUpExplorer::isStable(Timestamp t, InvariantId v) {
  return invariantStableAt.at(v) == t;
}
#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "core/idMap.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"
#include "exceptions/exceptions.hpp"

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

  template<bool DoCommit>
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

template<bool DoCommit>
void BottomUpExplorer::propagate(Timestamp currentTime) {
  // recursively expand variables to compute their value.
  while (varStackIdx_ > 0) {
    VarId currentVar = peekVariableStack();

    // If the variable is not stable, then expand it.
    if (!isStable(currentTime, currentVar)) {
      // Variable will become stable as it is either not defined or we now
      // expand its invariant. Note that expandInvariant may can sometimes not
      // push a new invariant nor a new variable on the stack, so we must mark
      // the variable as stable before we expand it as this otherwise results in
      // an infinite loop.
      markStable(currentTime, currentVar);
      // If the variable is not on the propagation path then ignore it.
      if (m_engine.isOnPropagationPath(currentVar) &&
          m_engine.getDefiningInvariant(currentVar) != NULL_ID) {
        // Variable is defined, on propagation path, and not stable, so expand
        // defining invariant.
        expandInvariant(m_engine.getDefiningInvariant(currentVar));
        continue;
      }
    }
    if (invariantStackIdx_ == 0) {
      popVariableStack();  // we are at an output variable that is already
                           // stable. Just continue!
      continue;
    }
    if (m_engine.hasChanged(currentTime, currentVar)) {
      // If the variable is stable and has changed then just send a
      // notification to top invariant (i.e, the one asking for its value)
      notifyCurrentInvariant();
    }
    bool invariantDone = visitNextVariable();
    if (invariantDone) {
      if constexpr (DoCommit) {
        m_engine.commitInvariant(peekInvariantStack());
      }
      // The top invariant has finished propagating, so all defined vars can
      // be marked as stable at the current time.
      for (auto defVar : m_engine.getVariablesDefinedBy(peekInvariantStack())) {
        markStable(currentTime, defVar);
        if constexpr (DoCommit) {
          m_engine.commit(defVar);
        }
      }
      popInvariantStack();
    }
  }
}
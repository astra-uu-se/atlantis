#pragma once

#include <vector>

#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "utils/idMap.hpp"

// Forward declare PropagationEngine
class PropagationEngine;

class OutputToInputExplorer {
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
  OutputToInputExplorer() = delete;
  OutputToInputExplorer(PropagationEngine& e, size_t expectedSize);

  void registerVar(VarId);
  void registerInvariant(InvariantId);
  /**
   * Register than we want to compute the value of v at time t
   */
  void registerForPropagation(Timestamp, VarId);

  void clearRegisteredVariables();

  template <bool OutputToInputMarking>
  void propagate(Timestamp currentTime);
};

inline void OutputToInputExplorer::registerForPropagation(Timestamp, VarId id) {
  // TODO: why not set varIsOnStack.at(v) = true;?
  // I remember that there was some technical reason but this need to be
  // documented. Note that this might overflow the stack otherwise.
  variableStack_[varStackIdx_++] = id;
}

inline void OutputToInputExplorer::clearRegisteredVariables() { varStackIdx_ = 0; }

inline void OutputToInputExplorer::pushVariableStack(VarId v) {
  varIsOnStack.set(v, true);
  variableStack_[varStackIdx_++] = v;
}
inline void OutputToInputExplorer::popVariableStack() {
  varIsOnStack.set(variableStack_[--varStackIdx_], false);
}
inline VarId OutputToInputExplorer::peekVariableStack() {
  return variableStack_[varStackIdx_ - 1];
}

inline void OutputToInputExplorer::pushInvariantStack(InvariantId i) {
  if (invariantIsOnStack.get(i)) {
    throw DynamicCycleException();
  }
  invariantIsOnStack.set(i, true);
  invariantStack_[invariantStackIdx_++] = i;
}
inline void OutputToInputExplorer::popInvariantStack() {
  invariantIsOnStack.set(invariantStack_[--invariantStackIdx_], false);
}

inline InvariantId OutputToInputExplorer::peekInvariantStack() {
  return invariantStack_[invariantStackIdx_ - 1];
}

inline void OutputToInputExplorer::markStable(Timestamp t, VarId v) {
  varStableAt[v] = t;
}

inline bool OutputToInputExplorer::isStable(Timestamp t, VarId v) {
  return varStableAt.at(v) == t;
}

inline bool OutputToInputExplorer::isStable(Timestamp t, InvariantId v) {
  return invariantStableAt.at(v) == t;
}

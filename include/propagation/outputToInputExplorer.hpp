#pragma once

#include <vector>

#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "utils/idMap.hpp"

// Forward declare PropagationEngine
class PropagationEngine;

class OutputToInputExplorer {
  PropagationEngine& _engine;

  std::vector<VarId> _variableStack;
  size_t _varStackIdx = 0;
  std::vector<InvariantId> _invariantStack;
  size_t _invariantStackIdx = 0;
  IdMap<VarId, Timestamp> _varStableAt;  // last timestamp when a VarID was
                                         // stable (i.e., will not change)
  IdMap<InvariantId, Timestamp> _invariantStableAt;
  IdMap<VarId, bool> _varIsOnStack;
  IdMap<InvariantId, bool> _invariantIsOnStack;

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
  // TODO: why not set _varIsOnStack.at(v) = true;?
  // I remember that there was some technical reason but this need to be
  // documented. Note that this might overflow the stack otherwise.
  _variableStack[_varStackIdx++] = id;
}

inline void OutputToInputExplorer::clearRegisteredVariables() {
  _varStackIdx = 0;
}

inline void OutputToInputExplorer::pushVariableStack(VarId v) {
  _varIsOnStack.set(v, true);
  _variableStack[_varStackIdx++] = v;
}
inline void OutputToInputExplorer::popVariableStack() {
  _varIsOnStack.set(_variableStack[--_varStackIdx], false);
}
inline VarId OutputToInputExplorer::peekVariableStack() {
  return _variableStack[_varStackIdx - 1];
}

inline void OutputToInputExplorer::pushInvariantStack(InvariantId i) {
  if (_invariantIsOnStack.get(i)) {
    throw DynamicCycleException();
  }
  _invariantIsOnStack.set(i, true);
  _invariantStack[_invariantStackIdx++] = i;
}
inline void OutputToInputExplorer::popInvariantStack() {
  _invariantIsOnStack.set(_invariantStack[--_invariantStackIdx], false);
}

inline InvariantId OutputToInputExplorer::peekInvariantStack() {
  return _invariantStack[_invariantStackIdx - 1];
}

inline void OutputToInputExplorer::markStable(Timestamp t, VarId v) {
  _varStableAt[v] = t;
}

inline bool OutputToInputExplorer::isStable(Timestamp t, VarId v) {
  return _varStableAt.at(v) == t;
}

inline bool OutputToInputExplorer::isStable(Timestamp t, InvariantId v) {
  return _invariantStableAt.at(v) == t;
}

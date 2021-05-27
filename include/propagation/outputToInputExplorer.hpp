#pragma once

#include <unordered_set>
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
  IdMap<VarIdBase, Timestamp> _varStableAt;  // last timestamp when a VarID was
                                             // stable (i.e., will not change)
  IdMap<InvariantId, Timestamp> _invariantStableAt;
  IdMap<VarIdBase, bool> _varIsOnStack;
  IdMap<InvariantId, bool> _invariantIsOnStack;

  IdMap<VarIdBase, std::unordered_set<size_t>> m_decisionVarAncestor;
  std::vector<VarIdBase> m_modifiedAncestors;

  template <bool OutputToInputMarking>
  void preprocessVarStack(Timestamp);

  template <bool OutputToInputMarking>
  bool isUpToDate(VarIdBase);

  void populateModifiedAncestors(Timestamp);
  void pushVariableStack(VarId v);
  void popVariableStack();
  VarId peekVariableStack();
  void pushInvariantStack(InvariantId);
  void popInvariantStack();
  InvariantId peekInvariantStack();
  void markStable(Timestamp, VarIdBase);
  bool isStable(Timestamp, VarIdBase);
  bool isStable(Timestamp, InvariantId);

  // We expand an invariant by pushing it and its first parameter variable onto
  // each stack.
  template <bool OutputToInputMarking>
  void expandInvariant(InvariantId inv);

  void notifyCurrentInvariant();

  template <bool OutputToInputMarking>
  bool visitNextVariable();

 public:
  OutputToInputExplorer() = delete;
  OutputToInputExplorer(PropagationEngine& engine, size_t expectedSize);

  void populateAncestors();
  void registerVar(VarId);
  void registerInvariant(InvariantId);
  /**
   * Register than we want to compute the value of v at timestamp ts
   */
  void registerForPropagation(Timestamp, VarId);

  void clearRegisteredVariables();

  template <bool OutputToInputMarking>
  void propagate(Timestamp);
};

inline void OutputToInputExplorer::registerForPropagation(Timestamp, VarId id) {
  pushVariableStack(id);
}

inline void OutputToInputExplorer::clearRegisteredVariables() {
  varStackIdx_ = 0;
}

inline void OutputToInputExplorer::pushVariableStack(VarId id) {
  _varIsOnStack.set(id, true);
  _variableStack[_varStackIdx++] = id;
}
inline void OutputToInputExplorer::popVariableStack() {
  _varIsOnStack.set(_variableStack[--_varStackIdx], false);
}
inline VarId OutputToInputExplorer::peekVariableStack() {
  return _variableStack[_varStackIdx - 1];
}

inline void OutputToInputExplorer::pushInvariantStack(InvariantId id) {
  if (_invariantIsOnStack.get(id)) {
    throw DynamicCycleException();
  }
  _invariantIsOnStack.set(id, true);
  _invariantStack[_invariantStackIdx++] = id;
}
inline void OutputToInputExplorer::popInvariantStack() {
  _invariantIsOnStack.set(_invariantStack[--_invariantStackIdx], false);
}

inline InvariantId OutputToInputExplorer::peekInvariantStack() {
  return _invariantStack[_invariantStackIdx - 1];
}

inline void OutputToInputExplorer::markStable(Timestamp ts, VarIdBase id) {
  _varStableAt[id] = ts;
}

inline bool OutputToInputExplorer::isStable(Timestamp ts, VarIdBase id) {
  return _varStableAt.at(id) == ts;
}

inline bool OutputToInputExplorer::isStable(Timestamp ts, InvariantId id) {
  return _invariantStableAt.at(id) == ts;
}

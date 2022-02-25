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
  IdMap<VarIdBase, Timestamp>
      _varComputedAt;  // last timestamp when a VarID was
                       // computed (i.e., will not change)
  IdMap<InvariantId, Timestamp> _invariantComputedAt;
  IdMap<InvariantId, bool> _invariantIsOnStack;

  IdMap<VarIdBase, std::unordered_set<VarIdBase>> _decisionVarAncestor;
  OutputToInputMarkingMode _outputToInputMarkingMode;

  template <OutputToInputMarkingMode MarkingMode>
  void preprocessVarStack([[maybe_unused]] Timestamp);

  template <OutputToInputMarkingMode MarkingMode>
  bool isMarked([[maybe_unused]] VarIdBase);

  void pushVariableStack(VarId);
  void popVariableStack();
  VarId peekVariableStack();
  void pushInvariantStack(InvariantId);
  void popInvariantStack();
  InvariantId peekInvariantStack();
  void setComputed(Timestamp, VarIdBase);
  bool isComputed(Timestamp, VarIdBase);
  bool isComputed(Timestamp, InvariantId);

  // We expand an invariant by pushing it and its first input variable onto
  // each stack.
  template <OutputToInputMarkingMode MarkingMode>
  void expandInvariant(InvariantId);
  void notifyCurrentInvariant();

  template <OutputToInputMarkingMode MarkingMode>
  bool pushNextInputVariable();
  template <OutputToInputMarkingMode MarkingMode>
  void propagate(Timestamp);

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

  void propagate(Timestamp);

  OutputToInputMarkingMode outputToInputMarkingMode() const;
  void setOutputToInputMarkingMode(OutputToInputMarkingMode);
};

inline void OutputToInputExplorer::registerForPropagation(Timestamp, VarId id) {
  pushVariableStack(id);
}

inline void OutputToInputExplorer::clearRegisteredVariables() {
  _varStackIdx = 0;
}

inline void OutputToInputExplorer::pushVariableStack(VarId id) {
  _variableStack[_varStackIdx++] = id;
}
inline void OutputToInputExplorer::popVariableStack() { --_varStackIdx; }
inline VarId OutputToInputExplorer::peekVariableStack() {
  return _variableStack[_varStackIdx - 1];
}

inline void OutputToInputExplorer::pushInvariantStack(InvariantId invariantId) {
  if (_invariantIsOnStack.get(invariantId)) {
    throw DynamicCycleException();
  }
  _invariantIsOnStack.set(invariantId, true);
  _invariantStack[_invariantStackIdx++] = invariantId;
}
inline void OutputToInputExplorer::popInvariantStack() {
  _invariantIsOnStack.set(_invariantStack[--_invariantStackIdx], false);
}

inline InvariantId OutputToInputExplorer::peekInvariantStack() {
  return _invariantStack[_invariantStackIdx - 1];
}

inline void OutputToInputExplorer::setComputed(Timestamp ts, VarIdBase id) {
  _varComputedAt[id] = ts;
}
inline bool OutputToInputExplorer::isComputed(Timestamp ts, VarIdBase id) {
  return _varComputedAt.at(id) == ts;
}

inline bool OutputToInputExplorer::isComputed(Timestamp ts,
                                              InvariantId invariantId) {
  return _invariantComputedAt.at(invariantId) == ts;
}

inline OutputToInputMarkingMode
OutputToInputExplorer::outputToInputMarkingMode() const {
  return _outputToInputMarkingMode;
}

inline void OutputToInputExplorer::setOutputToInputMarkingMode(
    OutputToInputMarkingMode markingMode) {
  _outputToInputMarkingMode = markingMode;
}

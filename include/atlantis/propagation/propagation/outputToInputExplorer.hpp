#pragma once

#include <unordered_set>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "propagation/types.hpp"
#include "propagation/utils/idMap.hpp"

namespace atlantis::propagation {

// Forward declare Solver
class Solver;

class OutputToInputExplorer {
  Solver& _solver;

  std::vector<VarId> _varStack;
  size_t _varStackIdx = 0;
  std::vector<InvariantId> _invariantStack;
  size_t _invariantStackIdx = 0;
  IdMap<VarIdBase, Timestamp>
      _varComputedAt;  // last timestamp when a VarID was
                       // computed (i.e., will not change)
  IdMap<InvariantId, Timestamp> _invariantComputedAt;
  IdMap<InvariantId, bool> _invariantIsOnStack;

  IdMap<VarIdBase, std::unordered_set<VarIdBase>> _searchVarAncestors;
  IdMap<VarIdBase, bool> _onPropagationPath;

  OutputToInputMarkingMode _outputToInputMarkingMode;

  template <OutputToInputMarkingMode MarkingMode>
  void preprocessVarStack([[maybe_unused]] Timestamp);

  template <OutputToInputMarkingMode MarkingMode>
  bool isMarked([[maybe_unused]] VarIdBase);

  void pushVarStack(VarId);
  void popVarStack();
  VarId peekVarStack();
  void pushInvariantStack(InvariantId);
  void popInvariantStack();
  InvariantId peekInvariantStack();
  void setComputed(Timestamp, VarIdBase);
  bool isComputed(Timestamp, VarIdBase);

  // We expand an invariant by pushing it and its first input variable onto
  // the stack.
  template <OutputToInputMarkingMode MarkingMode>
  void expandInvariant(InvariantId);
  void notifyCurrentInvariant();

  template <OutputToInputMarkingMode MarkingMode>
  bool pushNextInputVar();

  void outputToInputStaticMarking();
  void inputToOutputExplorationMarking();

  template <OutputToInputMarkingMode MarkingMode>
  void propagate(Timestamp);

 public:
  OutputToInputExplorer() = delete;
  OutputToInputExplorer(Solver& solver, size_t expectedSize);

  void registerVar(VarId);
  void registerInvariant(InvariantId);
  /**
   * Register than we want to compute the value of v at timestamp ts
   */
  void registerForPropagation(Timestamp, VarId);

  void clearRegisteredVars();

  void propagate(Timestamp);

  [[nodiscard]] OutputToInputMarkingMode outputToInputMarkingMode() const;
  void setOutputToInputMarkingMode(OutputToInputMarkingMode);

  template <OutputToInputMarkingMode MarkingMode>
  void close();
};

inline void OutputToInputExplorer::registerForPropagation(Timestamp, VarId id) {
  pushVarStack(id);
}

inline void OutputToInputExplorer::clearRegisteredVars() { _varStackIdx = 0; }

inline void OutputToInputExplorer::pushVarStack(VarId id) {
  _varStack[_varStackIdx++] = id;
}
inline void OutputToInputExplorer::popVarStack() { --_varStackIdx; }
inline VarId OutputToInputExplorer::peekVarStack() {
  return _varStack[_varStackIdx - 1];
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

inline OutputToInputMarkingMode
OutputToInputExplorer::outputToInputMarkingMode() const {
  return _outputToInputMarkingMode;
}

inline void OutputToInputExplorer::setOutputToInputMarkingMode(
    OutputToInputMarkingMode markingMode) {
  _outputToInputMarkingMode = markingMode;
}

}  // namespace atlantis::propagation
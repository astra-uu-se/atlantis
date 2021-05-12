
#include "propagation/outputToInputExplorer.hpp"

#include "core/propagationEngine.hpp"

OutputToInputExplorer::OutputToInputExplorer(PropagationEngine& e,
                                             size_t expectedSize)
    : _engine(e),
      _varStackIdx(0),
      _invariantStackIdx(0),
      _varStableAt(expectedSize),
      _invariantStableAt(expectedSize),
      _varIsOnStack(expectedSize),
      _invariantIsOnStack(expectedSize) {
  _variableStack.reserve(expectedSize);
  _invariantStack.reserve(expectedSize);
}

// We expand an invariant by pushing it and its first input variable onto each
// stack.
void OutputToInputExplorer::expandInvariant(InvariantId inv) {
  if (_invariantIsOnStack.get(inv)) {
    throw DynamicCycleException();
  }
  VarId nextVar = _engine.getNextDependency(inv);
  // Ignore var if it is not on propagation path.
  while (nextVar != NULL_ID && !_engine.isOnPropagationPath(nextVar)) {
    nextVar = _engine.getNextDependency(inv);
  }
  if (nextVar.id == NULL_ID) {
    return;
  }
  pushVariableStack(nextVar);
  pushInvariantStack(inv);
}

void OutputToInputExplorer::notifyCurrentInvariant() {
  _engine.notifyCurrentDependencyChanged(peekInvariantStack());
}

bool OutputToInputExplorer::visitNextVariable() {
  popVariableStack();
  VarId nextVar = _engine.getNextDependency(peekInvariantStack());
  while (nextVar != NULL_ID && !_engine.isOnPropagationPath(nextVar)) {
    nextVar = _engine.getNextDependency(peekInvariantStack());
  }
  if (nextVar.id == NULL_ID) {
    return true;  // done with invariant
  }
  pushVariableStack(nextVar);
  return false;  // not done with invariant
}

void OutputToInputExplorer::registerVar(VarId id) {
  _variableStack.emplace_back(NULL_ID);  // push back just to resize the stack!
  _varStableAt.register_idx(id);
  _varIsOnStack.register_idx(id, false);
}

void OutputToInputExplorer::registerInvariant(InvariantId id) {
  _invariantStack.emplace_back(NULL_ID);  // push back just to resize the stack!
  _invariantStableAt.register_idx(id);
  _invariantIsOnStack.register_idx(id, false);
}

template void OutputToInputExplorer::propagate<true>(Timestamp currentTime);
template void OutputToInputExplorer::propagate<false>(Timestamp currentTime);

template <bool DoCommit>
void OutputToInputExplorer::propagate(Timestamp currentTime) {
  // recursively expand variables to compute their value.
  while (_varStackIdx > 0) {
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
      if (_engine.isOnPropagationPath(currentVar) &&
          _engine.getDefiningInvariant(currentVar) != NULL_ID) {
        // Variable is defined, on propagation path, and not stable, so expand
        // defining invariant.
        expandInvariant(_engine.getDefiningInvariant(currentVar));
        continue;
      }
    }
    if (_invariantStackIdx == 0) {
      popVariableStack();  // we are at an output variable that is already
                           // stable. Just continue!
      continue;
    }
    if (_engine.hasChanged(currentTime, currentVar)) {
      // If the variable is stable and has changed then just send a
      // notification to top invariant (i.e, the one asking for its value)
      notifyCurrentInvariant();
    }
    bool invariantDone = visitNextVariable();
    if (invariantDone) {
      if constexpr (DoCommit) {
        _engine.commitInvariant(peekInvariantStack());
      }
      // The top invariant has finished propagating, so all defined vars can
      // be marked as stable at the current time.
      for (auto defVar : _engine.getVariablesDefinedBy(peekInvariantStack())) {
        markStable(currentTime, defVar);
        if constexpr (DoCommit) {
          _engine.commit(defVar);
        }
      }
      popInvariantStack();
    }
  }
}

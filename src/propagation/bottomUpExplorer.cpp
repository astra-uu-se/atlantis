
#include "propagation/bottomUpExplorer.hpp"

#include "core/propagationEngine.hpp"

BottomUpExplorer::BottomUpExplorer(PropagationEngine& e, size_t expectedSize)
    : m_engine(e),
      varStackIdx_(0),
      invariantStackIdx_(0),
      varStableAt(expectedSize),
      invariantStableAt(expectedSize),
      varIsOnStack(expectedSize),
      invariantIsOnStack(expectedSize) {
  variableStack_.reserve(expectedSize);
  invariantStack_.reserve(expectedSize);
}

// We expand an invariant by pushing it and its first input variable onto each
// stack.
void BottomUpExplorer::expandInvariant(InvariantId inv) {
  if (invariantIsOnStack.get(inv)) {
    throw DynamicCycleException();
  }
  VarId nextVar = m_engine.getNextDependency(inv);
  // Ignore var if it is not on propagation path.
  while (nextVar != NULL_ID && !m_engine.isOnPropagationPath(nextVar)) {
    nextVar = m_engine.getNextDependency(inv);
  }
  if (nextVar.id == NULL_ID) {
    return;
  }
  pushVariableStack(nextVar);
  pushInvariantStack(inv);
}

void BottomUpExplorer::notifyCurrentInvariant() {
  m_engine.notifyCurrentDependencyChanged(peekInvariantStack());
}

bool BottomUpExplorer::visitNextVariable() {
  popVariableStack();
  VarId nextVar = m_engine.getNextDependency(peekInvariantStack());
  while (nextVar != NULL_ID && !m_engine.isOnPropagationPath(nextVar)) {
    nextVar = m_engine.getNextDependency(peekInvariantStack());
  }
  if (nextVar.id == NULL_ID) {
    return true;  // done with invariant
  }
  pushVariableStack(nextVar);
  return false;  // not done with invariant
}

void BottomUpExplorer::registerVar(VarId id) {
  variableStack_.emplace_back(NULL_ID);  // push back just to resize the stack!
  varStableAt.register_idx(id);
  varIsOnStack.register_idx(id, false);
}

void BottomUpExplorer::registerInvariant(InvariantId id) {
  invariantStack_.emplace_back(NULL_ID);  // push back just to resize the stack!
  invariantStableAt.register_idx(id);
  invariantIsOnStack.register_idx(id, false);
}

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
      // The top invariant has finished propagating, so all defined vars can
      // be marked as stable at the current time.
      for (auto defVar : m_engine.getVariablesDefinedBy(peekInvariantStack())) {
        markStable(currentTime, defVar);
      }
      popInvariantStack();
    }
  }
}

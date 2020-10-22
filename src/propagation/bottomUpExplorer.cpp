
#include "propagation/bottomUpExplorer.hpp"

#include "core/propagationEngine.hpp"

BottomUpExplorer::BottomUpExplorer(PropagationEngine& e, size_t expectedSize)
    : m_engine(e), varStackIdx_(0), invariantStackIdx_(0) {
  variableStack_.reserve(expectedSize);
  invariantStack_.reserve(expectedSize);
  varStableAt.reserve(expectedSize);
  invariantStableAt.reserve(expectedSize);
  varIsOnStack.reserve(expectedSize);
  invariantIsOnStack.reserve(expectedSize);

  // add null entry.
  varStableAt.push_back(NULL_TIMESTAMP);
  invariantStableAt.push_back(NULL_TIMESTAMP);
  varIsOnStack.push_back(false);
  invariantIsOnStack.push_back(false);
}

// We expand an invariant by pushing it and its first input variable onto each
// stack.
void BottomUpExplorer::expandInvariant(InvariantId inv) {
  if (invariantIsOnStack.at(inv)) {
    throw DynamicCycleException();
  }
  // TODO: ignore variable if not on propagationPath.
  VarId nextVar = m_engine.getNextDependency(inv);
  assert(nextVar.id !=
         NULL_ID);  // Invariant must have at least one dependency, and this
                    // should be the first (and only) time we expand it
  assert(nextVar.idType == VarIdType::var);
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
  } else {
    pushVariableStack(nextVar);
    return false;  // not done with invariant
  }
}

void BottomUpExplorer::registerVar(VarId) {
  variableStack_.push_back(NULL_ID);  // push back just to resize the stack!
  varStableAt.push_back(NULL_TIMESTAMP);
  varIsOnStack.push_back(false);
}

void BottomUpExplorer::registerInvariant(InvariantId) {
  invariantStack_.push_back(NULL_ID);  //  push back just to resize the stack!
  invariantStableAt.push_back(NULL_TIMESTAMP);
  invariantIsOnStack.push_back(false);
}

void BottomUpExplorer::propagate(Timestamp currentTime) {
  // recursively expand variables to compute their value.
  while (varStackIdx_ > 0) {
    VarId currentVar = peekVariableStack();
    // If the variable is not stable, then expand it.
    if (!isStable(currentTime, currentVar)) {
      if (m_engine.getDefiningInvariant(currentVar) != NULL_ID) {
        // Variable is defined and not stable, so expand defining invariant.
        expandInvariant(m_engine.getDefiningInvariant(currentVar));
        continue;
      }
      markStable(currentTime, currentVar);
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

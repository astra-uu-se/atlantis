
#include "propagation/bottomUpExplorer.hpp"

#include "core/propagationEngine.hpp"

// We expand an invariant by pushing it and its first input variable onto each
// stack.
void BottomUpExplorer::expandInvariant(InvariantId inv) {
  if (invariantIsOnStack.at(inv)) {
    throw DynamicCycleException();
  }
  VarId nextVar = m_engine.getNextDependency(inv);
  assert(nextVar.id !=
         NULL_ID);  // Invariant must have at least one dependency, and this
                    // should be the first (and only) time we expand it
  pushVariableStack(nextVar);
  pushInvariantStack(inv);
}

void BottomUpExplorer::notifyCurrentInvariant() {
  m_engine.notifyCurrentDependencyChanged(peekInvariantStack());
}

bool BottomUpExplorer::visitNextVariable() {
  popVariableStack();
  VarId nextVar = m_engine.getNextDependency(peekInvariantStack());
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
#include "propagation/bottomUpPropagationGraph.hpp"

#include "core/propagationEngine.hpp"

BottomUpPropagationGraph::BottomUpPropagationGraph(PropagationEngine& e,
                                                   size_t expectedSize)
    : PropagationGraph(expectedSize),
      m_engine(e),
      varStackIdx_(0),
      invariantStackIdx_(0) {
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

void BottomUpPropagationGraph::notifyMaybeChanged(Timestamp, VarId) {}

void BottomUpPropagationGraph::clearForPropagation() {
  // size_t vsSize = variableStack_.capacity();
  // variableStack_.clear();
  // variableStack_.reserve(
  //     vsSize);  // The c++ specifications do not say if clear
  //               // will keep the reserved size unchanged, so worse case this
  //               // will do a reallocation per propagation.
  // size_t isSize = invariantStack_.capacity();
  // invariantStack_.clear();
  // invariantStack_.reserve(isSize);
  // varIsOnStack.assign(varIsOnStack.size(), false);
  // invariantIsOnStack.assign(invariantIsOnStack.size(), false);
}

void BottomUpPropagationGraph::registerForPropagation(Timestamp, VarId id) {
  variableStack_[varStackIdx_++] = id;
}

void BottomUpPropagationGraph::fullPropagateAndCommit(Timestamp currentTime) {
  // TODO: make this variable ID hacking independent

  // Query all variables, propagate, and commit every variable and invariant.
  for (size_t i = 1; i < m_numVariables + 1; i++) {
    variableStack_[varStackIdx_++] = VarId(i);
  }
  propagate(currentTime);
  for (size_t i = 1; i < m_numVariables + 1; i++) {
    m_engine.commitIf(currentTime, VarId(i));
  }
  for (size_t i = 1; i < m_numInvariants + 1; i++) {
    m_engine.commitInvariantIf(currentTime, InvariantId(i));
  }
}

void BottomUpPropagationGraph::propagate(Timestamp currentTime) {
  // recursively expand variables to compute their value.
  while (varStackIdx_ > 0) {
    VarId currentVar = peekVariableStack();
    // If the variable is not stable, then expand it.
    if (!isStable(currentTime, currentVar)) {
      if (m_definingInvariant.at(currentVar).id != NULL_ID) {
        // Variable is defined and not stable, so expand defining invariant.
        expandInvariant(m_definingInvariant.at(currentVar));
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
      for (auto defVar : variablesDefinedBy(peekInvariantStack())) {
        markStable(currentTime, defVar);
      }
      popInvariantStack();
    }
  }
}

void BottomUpPropagationGraph::lazyPropagateAndCommit(Timestamp currentTime) {
  propagate2(currentTime);
  for (size_t i = 1; i < m_numVariables + 1; i++) {
    VarId id = VarId(i);
    // Commit all variables that were propagated and the input variables.
    if (isStable(currentTime, id) || isInputVar(id)) {
      commitAndPostpone(currentTime, id);
    }
  }
  for (size_t i = 1; i < m_numInvariants + 1; i++) {
    InvariantId id = InvariantId(i);
    if (isStable(currentTime, id)) {
      // Commit all invariants that were propagated.
      m_engine.commitInvariantIf(currentTime, id);
    }
  }
}

/**
 * When we use lazy commit, the propagate algorithm has to perform recomputation
 */
void BottomUpPropagationGraph::propagate2(Timestamp currentTime) {
  // recursively expand variables to compute their value.
  while (varStackIdx_ > 0) {
    VarId currentVar = peekVariableStack();
    // If the variable is not stable, then expand it.
    if (!isStable(currentTime, currentVar)) {
      if (m_definingInvariant.at(currentVar).id != NULL_ID) {
        // Variable is defined and not stable, so expand defining invariant.
        expandInvariant(m_definingInvariant.at(currentVar));
        continue;
      }
      markStable(currentTime, currentVar);
    }
    if (invariantStackIdx_ == 0) {
      popVariableStack();  // we are at an output variable that is already
                           // stable. Just continue!
      continue;
    }
    if (!m_engine.isPostponed(peekInvariantStack()) &&
        m_engine.hasChanged(currentTime, currentVar)) {
      // If the variable is stable and has changed then send a notification
      // to top invariant (i.e, the one asking for its value).
      // If the top invariant is postponed, then it will be recomputed and
      // we do not have to notify it.
      notifyCurrentInvariant();
    }
    bool inputDone = visitNextVariable();
    if (inputDone) {
      // Input variables are now up to date.
      // If the invariant was postponed then recompute it.
      if (m_engine.isPostponed(peekInvariantStack())) {
        // recompute at the current timestamp
        m_engine.recompute(peekInvariantStack());
      }
      for (auto defVar : variablesDefinedBy(peekInvariantStack())) {
        markStable(currentTime, defVar);
      }
      markStable(currentTime, peekInvariantStack());
      popInvariantStack();
    }
  }
}

inline void BottomUpPropagationGraph::commitAndPostpone(Timestamp t, VarId id) {
  m_engine.commitIf(t, id);
  for (auto depInvariant : m_listeningInvariants.at(id)) {
    m_engine.postpone(depInvariant);
  }
}
// We expand an invariant by pushing it and its first input variable onto each
// stack.
inline void BottomUpPropagationGraph::expandInvariant(InvariantId inv) {
  // TODO:Check if invariant is already on stack.
  VarId nextVar = m_engine.getNextDependency(inv);
  assert(nextVar.id !=
         NULL_ID);  // Invariant must have at least one dependency, and this
                    // should be the first (and only) time we expand it
  pushVariableStack(nextVar);
  pushInvariantStack(inv);
}

inline void BottomUpPropagationGraph::notifyCurrentInvariant() {
  m_engine.notifyCurrentDependencyChanged(peekInvariantStack());
}

inline bool BottomUpPropagationGraph::visitNextVariable() {
  popVariableStack();
  VarId nextVar = m_engine.getNextDependency(peekInvariantStack());
  if (nextVar.id == NULL_ID) {
    return true;  // done with invariant
  } else {
    pushVariableStack(nextVar);
    return false;  // not done with invariant
  }
}

void BottomUpPropagationGraph::registerVar(VarId id) {
  PropagationGraph::registerVar(id);  // call parent implementation
  variableStack_.push_back(NULL_ID);  // push back just to resize the stack!
  varStableAt.push_back(NULL_TIMESTAMP);
  varIsOnStack.push_back(false);
}

void BottomUpPropagationGraph::registerInvariant(InvariantId id) {
  PropagationGraph::registerInvariant(id);  // call parent implementation
  invariantStack_.push_back(NULL_ID);  //  push back just to resize the stack!
  invariantStableAt.push_back(NULL_TIMESTAMP);
  invariantIsOnStack.push_back(false);
}

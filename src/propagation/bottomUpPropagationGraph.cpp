#include "propagation/bottomUpPropagationGraph.hpp"

#include "core/engine.hpp"

BottomUpPropagationGraph::BottomUpPropagationGraph(Engine& e,
                                                   size_t expectedSize)
    : PropagationGraph(expectedSize), m_engine(e) {
  variableStack.reserve(expectedSize);
  invariantStack.reserve(expectedSize);
  stableAt.reserve(expectedSize);
  varIsOnStack.reserve(expectedSize);
  invariantIsOnStack.reserve(expectedSize);

  // add null entry.
  stableAt.push_back(NULL_TIMESTAMP);
  varIsOnStack.push_back(false);
  invariantIsOnStack.push_back(false);
}

void BottomUpPropagationGraph::notifyMaybeChanged(Timestamp, VarId) {}

void BottomUpPropagationGraph::clearForPropagation() {
  size_t vsSize = variableStack.capacity();
  variableStack.clear();
  variableStack.reserve(
      vsSize);  // The c++ specifications do not say if clear
                // will keep the reserved size unchanged, so worse case this
                // will do a reallocation per propagation.
  size_t isSize = invariantStack.capacity();
  invariantStack.clear();
  invariantStack.reserve(isSize);
  varIsOnStack.assign(varIsOnStack.size(), false);
  invariantIsOnStack.assign(invariantIsOnStack.size(), false);
}
void BottomUpPropagationGraph::registerForPropagation(Timestamp, VarId id) {
  variableStack.push_back(id);
}

void BottomUpPropagationGraph::fullPropagateAndCommit(Timestamp currentTime) {
  // TODO: make this variable ID hacking independent

  // Query all variables, propagate, and commit every variable and invariant.
  for (size_t i = 1; i < m_numVariables + 1; i++) {
    variableStack.push_back(VarId(i));
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
  while (!variableStack.empty()) {
    VarId currentVar = variableStack.back();
    // If the variable is not stable, then expand it.
    if (!isStable(currentTime, currentVar)) {
      if (m_definingInvariant.at(currentVar).id != NULL_ID) {
        // Variable is defined and not stable, so expand defining invariant.
        expandInvariant(m_definingInvariant.at(currentVar));
        continue;
      } else {
        markStable(currentTime, currentVar);
        // continue;
        goto handle_stable_var;  // Don't panic.
      }
    } else {  // If stable
    handle_stable_var:
      if (invariantStack.empty()) {
        popVariableStack();  // we are at an output variable that is already
                             // stable. Just continue!
        continue;
      } else {
        if (m_engine.hasChanged(currentTime, currentVar)) {
          // If the variable is stable and has changed then just send a
          // notification to top invariant (i.e, the one asking for its value)
          notifyCurrentInvariant();
        }
        bool invariantDone = visitNextVariable();
        if (invariantDone) {
          // The top invariant has finished propagating, so all defined vars can
          // be marked as stable at the current time.
          for (auto defVar : variableDefinedBy(invariantStack.back())) {
            markStable(currentTime, defVar);
          }
          popInvariantStack();
        }
        continue;
      }
    }
  }
}

inline void BottomUpPropagationGraph::pushVariableStack(VarId v) {
  if (varIsOnStack.at(v)) {
    throw DynamicCycleException();
  }
  varIsOnStack.at(v) = true;
  variableStack.push_back(v);
}
inline void BottomUpPropagationGraph::popVariableStack() {
  varIsOnStack.at(variableStack.back()) = false;
  variableStack.pop_back();
}

inline void BottomUpPropagationGraph::pushInvariantStack(InvariantId i) {
  if (invariantIsOnStack.at(i)) {
    assert(false);  // Dynamic cycle!
    // TODO: throw exception.
    // TODO: do we need to clean up? I don't think we do!
  }
  invariantIsOnStack.at(i) = true;
  invariantStack.push_back(i);
}
inline void BottomUpPropagationGraph::popInvariantStack() {
  invariantIsOnStack.at(invariantStack.back()) = false;
  invariantStack.pop_back();
}

inline void BottomUpPropagationGraph::markStable(Timestamp t, VarId v) {
  stableAt.at(v) = t;
}

inline bool BottomUpPropagationGraph::isStable(Timestamp t, VarId v) {
  return stableAt.at(v) == t;
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
  m_engine.notifyCurrentDependencyChanged(invariantStack.back());
}

inline bool BottomUpPropagationGraph::visitNextVariable() {
  popVariableStack();
  VarId nextVar = m_engine.getNextDependency(invariantStack.back());
  if (nextVar.id == NULL_ID) {
    return true;  // done with invariant
  } else {
    pushVariableStack(nextVar);
    return false;  // not done with invariant
  }
}

void BottomUpPropagationGraph::registerVar(VarId id) {
  PropagationGraph::registerVar(id);  // call parent implementation
  variableStack.push_back(NULL_ID);   // push back just to resize the stack!
  stableAt.push_back(NULL_TIMESTAMP);
  varIsOnStack.push_back(false);
}

void BottomUpPropagationGraph::registerInvariant(InvariantId id) {
  PropagationGraph::registerInvariant(id);  // call parent implementation
  invariantStack.push_back(NULL_ID);  // push back just to resize the stack!
  invariantIsOnStack.push_back(false);
}
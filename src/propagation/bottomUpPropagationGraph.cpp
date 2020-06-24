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

void BottomUpPropagationGraph::notifyMaybeChanged(const Timestamp&, VarId) {}

void BottomUpPropagationGraph::clearForPropagation() {
  size_t vsSize = variableStack.capacity();
  variableStack.clear();
  variableStack.reserve(
      vsSize);  // The c++ specifications do not say if clear
                // will keep the reserved size unchanged, so worse case this
                // will once do the reallocation.
  size_t isSize = invariantStack.capacity();
  invariantStack.clear();
  invariantStack.reserve(isSize);
  // stableAt.assign(stableAt.size(), (Timestamp)NULL_TIMESTAMP); // No need to
  // clear due to timestamping!
  varIsOnStack.assign(varIsOnStack.size(), false);
  invariantIsOnStack.assign(invariantIsOnStack.size(), false);
}
void BottomUpPropagationGraph::registerForPropagation(const Timestamp&,
                                                      VarId id) {
  variableStack.push_back(id);
}
void BottomUpPropagationGraph::propagate(const Timestamp& currentTime) {
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
        goto handle_stable_var;  // Don't panic.
      }
    } else if (invariantStack.empty()) {
      popVariableStack();  // we are at an output variable that is already
                           // stable. Just continue!
      continue;
    } else {  // If stable
    handle_stable_var:
      if (m_engine.hasChanged(currentTime, currentVar)) {
        // If the variable is stable and has changed then just send a
        // notification to top invariant (i.e, the one asking for its value)
        notifyCurrentInvariant();
        visitNextVariable(currentTime);
        continue;
      } else {
        // pop
        visitNextVariable(currentTime);
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

inline void BottomUpPropagationGraph::markStable(const Timestamp& t, VarId v) {
  stableAt.at(v) = t;
}

inline bool BottomUpPropagationGraph::isStable(const Timestamp& t, VarId v) {
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

// TODO: this will push a variable onto the stack even when the variable is
// stable. The reason for this is that we will then have to notify the top
// invariant that the variable is stable (in case it has changed).
inline void BottomUpPropagationGraph::visitNextVariable(
    const Timestamp& currentTime) {
  popVariableStack();
  VarId nextVar = m_engine.getNextDependency(invariantStack.back());
  if (nextVar.id == NULL_ID) {
    // The top invariant has finished propagating, so all defined vars can be
    // marked as stable at the current time.
    for (auto defVar : variableDefinedBy(invariantStack.back())) {
      markStable(currentTime, defVar);
    }
    popInvariantStack();
  } else {
    pushVariableStack(nextVar);
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
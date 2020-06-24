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
void BottomUpPropagationGraph::propagate() {
  // recursively expand variables to compute their value.
  while (!variableStack.empty()) {
    // std::cout << " ---- \n";
    // printVarStack();
    // printInvariantStack();

    VarId currentVar = variableStack.back();
    // If the variable is not stable, then expand it.
    if (!isStable(m_engine.getCurrentTime(), currentVar)) {
      if (m_definingInvariant.at(currentVar).id != NULL_ID) {
        // Variable is defined and not stable, so expand defining invariant.
        expandInvariant(m_definingInvariant.at(currentVar));
        continue;
      } else if (m_engine.hasChanged(m_engine.getCurrentTime(), currentVar)) {
        // The variable is not defined so if it has changed, then we notify the
        // top invariant. Note that this must be an input variable.
        // TODO: just pre-mark all the input variables as stable when modified
        // in a move and remove this case!
        notifyCurrentInvariant(currentVar);
        markStable(m_engine.getCurrentTime(), currentVar);
        visitNextVariable();
        continue;
      } else {
        markStable(m_engine.getCurrentTime(), currentVar);
        visitNextVariable();
        continue;
      }
    } else if (invariantStack.empty()) {
      popVariableStack();  // we are at an output variable that is already
                           // stable. Just continue!
      continue;
    } else {  // If stable
      if (m_engine.hasChanged(m_engine.getCurrentTime(), currentVar)) {
        // If the variable is stable and has changed then just send a
        // notification to top invariant (i.e, the one asking for its value)
        // TODO: prevent this case from happening by checking if a variables is
        // marked and has changed before pushing it onto the stack? Not clear
        // how much faster that would actually be.
        notifyCurrentInvariant(currentVar);
        visitNextVariable();
        continue;
      } else {
        // pop
        visitNextVariable();
        continue;
      }
    }
  }
}

inline void BottomUpPropagationGraph::pushVariableStack(VarId v) {
  if (varIsOnStack.at(v)) {
    assert(false);  // Dynamic cycle!
    // TODO: throw exception.
    // TODO: do we need to clean up? I don't think we do!
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
  VarId nextVar = m_engine.getStore().getInvariant(inv).getNextDependency(
      m_engine.getCurrentTime());
  assert(nextVar.id !=
         NULL_ID);  // Invariant must have at least one dependency, and this
                    // should be the first (and only) time we expand it
  pushVariableStack(nextVar);
  pushInvariantStack(inv);
}

inline void BottomUpPropagationGraph::notifyCurrentInvariant(VarId id) {
  const IntVar variable = m_engine.getStore().getConstIntVar(id);
  m_engine.getStore()
      .getInvariant(invariantStack.back())
      .notifyCurrentDependencyChanged(
          m_engine.getCurrentTime(), m_engine, variable.getCommittedValue(),
          variable.getValue(m_engine.getCurrentTime()));
}

// TODO: this will push a variable onto the stack even when the variable is
// stable. The reason for this is that we will then have to notify the top
// invariant that the variable is stable (in case it has changed).
inline void BottomUpPropagationGraph::visitNextVariable() {
  popVariableStack();
  VarId nextVar = m_engine.getStore()
                      .getInvariant(invariantStack.back())
                      .getNextDependency(m_engine.getCurrentTime());
  if (nextVar.id == NULL_ID) {
    // The top invariant has finished propagating, so all defined vars can be
    // marked as stable at the current time.
    for (auto defVar : variableDefinedBy(invariantStack.back())) {
      markStable(m_engine.getCurrentTime(), defVar);
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
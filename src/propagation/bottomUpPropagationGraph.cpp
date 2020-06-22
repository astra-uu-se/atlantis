#include "propagation/bottomUpPropagationGraph.hpp"

#include "core/engine.hpp"

BottomUpPropagationGraph::BottomUpPropagationGraph(Engine& e,
                                                   size_t expectedSize)
    : PropagationGraph(expectedSize), m_engine(e) {
  variableStack.reserve(expectedSize);
  invariantStack.reserve(expectedSize);
  stableAt.reserve(expectedSize);
  isOnStack.reserve(expectedSize);

  // add null entry.
  stableAt.push_back(NULL_TIMESTAMP);
  isOnStack.push_back(false);
}

void BottomUpPropagationGraph::notifyMaybeChanged(const Timestamp&,
                                                  VarId) {}

void BottomUpPropagationGraph::clearForPropagation() {
  variableStack.clear();
  invariantStack.clear();
  // stableAt.assign(stableAt.size(), (Timestamp)NULL_TIMESTAMP); // No need to
  // clear due to timestamping!
  isOnStack.assign(isOnStack.size(), false);
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
        // Variable is defined, so expand defining invariant.
        expandInvariant(m_definingInvariant.at(currentVar));
        continue;
      } else if (m_engine.hasChanged(m_engine.getCurrentTime(), currentVar)) {
        // The variable is not defined so if it has changed, then we notify the
        // top invariant. Note that this must be an input variable.
        // TODO: just pre-mark all the input variables as stable and remove this
        // case!
        notifyCurrentInvariant(currentVar);
        nextVar();
        continue;
      } else {
        nextVar();
        continue;
      }
    } else if (invariantStack.empty()) {
      popStack();  // we are at an output variable that is already stable. Just
                   // continue!
      continue;
    } else {
      if (m_engine.hasChanged(m_engine.getCurrentTime(), currentVar)) {
        // Else, if the variable has been marked before and has changed then
        // just send a notification to top invariant (i.e, the one asking for
        // its value)
        // TODO: prevent this case from happening by checking if a variables is
        // marked and has changed before pushing it onto the stack.
        notifyCurrentInvariant(currentVar);
        nextVar();
        continue;
      } else {
        // pop
        nextVar();
        continue;
      }
    }
  }
}

inline void BottomUpPropagationGraph::pushStack(VarId v) {
  isOnStack.at(v) = true;
  variableStack.push_back(v);
}
inline void BottomUpPropagationGraph::popStack() {
  isOnStack.at(variableStack.back()) = false;
  variableStack.pop_back();
}
inline void BottomUpPropagationGraph::stable(const Timestamp& t, VarId v) {
  stableAt.at(v) = t;
}

inline bool BottomUpPropagationGraph::isStable(const Timestamp& t, VarId v) {
  return stableAt.at(v) == t;
}

// We expand an invariant by pushing it and its first input variable onto each
// stack.
inline void BottomUpPropagationGraph::expandInvariant(InvariantId inv) {
  VarId nextVar = m_engine.getStore().getInvariant(inv).getNextDependency(
      m_engine.getCurrentTime());
  assert(nextVar.id !=
         NULL_ID);  // Invariant must have at least one dependency, and this
                    // should be the first (and only) time we expand it
  pushStack(nextVar);
  invariantStack.push_back(inv);
}

inline void BottomUpPropagationGraph::notifyCurrentInvariant(VarId id) {
  IntVar variable = m_engine.getStore().getConstIntVar(id);
  m_engine.getStore()
      .getInvariant(invariantStack.back())
      .notifyCurrentDependencyChanged(
          m_engine.getCurrentTime(), m_engine, variable.getCommittedValue(),
          variable.getValue(m_engine.getCurrentTime()));
}

inline void BottomUpPropagationGraph::nextVar() {
  popStack();
  VarId nextVar = m_engine.getStore()
                      .getInvariant(invariantStack.back())
                      .getNextDependency(m_engine.getCurrentTime());
  if (nextVar.id == NULL_ID) {
    // The invariant has finished propagating, so all defined vars can be
    // marked as up to date.
    // Do this with member array of timestamps.
    for (auto defVar : variableDefinedBy(invariantStack.back())) {
      stable(m_engine.getCurrentTime(), defVar);
    }
    invariantStack.pop_back();
  } else {
    if (isOnStack.at(nextVar)) {
      assert(false);
      // TODO: throw exception.
      // TODO: do we need to clean up?
    }
    pushStack(nextVar);
  }
}

// VarId BottomUpPropagationGraph::getNextStableVariable(const Timestamp& t) {
// }

void BottomUpPropagationGraph::registerVar(VarId id) {
  PropagationGraph::registerVar(id);  // call parent implementation
  variableStack.push_back(NULL_ID);   // push back just to resize the stack!
  stableAt.push_back(NULL_TIMESTAMP);
  isOnStack.push_back(false);
}

void BottomUpPropagationGraph::registerInvariant(InvariantId id) {
  PropagationGraph::registerInvariant(id);  // call parent implementation
  invariantStack.push_back(NULL_ID);  // push back just to resize the stack!
}
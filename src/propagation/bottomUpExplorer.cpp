
#include "propagation/bottomUpExplorer.hpp"

#include "core/propagationEngine.hpp"

BottomUpExplorer::BottomUpExplorer(PropagationEngine& e, size_t expectedSize)
    : m_engine(e),
      varStackIdx_(0),
      invariantStackIdx_(0),
      varStableAt(expectedSize),
      invariantStableAt(expectedSize),
      varIsOnStack(expectedSize),
      invariantIsOnStack(expectedSize),
      m_decisionVarAncestor(expectedSize) {
  variableStack_.reserve(expectedSize);
  invariantStack_.reserve(expectedSize);
}

void BottomUpExplorer::populateAncestors() {
  std::vector<bool> varVisited(m_engine.getNumVariables() + 1);
  std::deque<IdBase> stack(m_engine.getNumVariables() + 1);

  for (IdBase idx = 1; idx <= m_engine.getNumVariables(); ++idx) {
    m_decisionVarAncestor.register_idx(idx);
    m_decisionVarAncestor[idx].clear();
    m_decisionVarAncestor[idx].reserve(
        m_engine.getDecisionVariables().size());
  }

  varVisited.resize(m_engine.getNumVariables() + 1);

  for (const VarIdBase decisionVar : m_engine.getDecisionVariables()) {
    std::fill(varVisited.begin(), varVisited.end(), NULL_ID);
    stack.clear();
    stack.push_back(decisionVar);
    varVisited[decisionVar] = true;

    while (stack.size() > 0) {
      const VarIdBase id = stack.back();
      stack.pop_back();
      m_decisionVarAncestor[id].emplace(decisionVar);
      
      for (InvariantId invariantId : m_engine.getInvariantsDefinedBy(IdBase(id))) {
        for (VarIdBase outputVar : m_engine.getVariablesDefinedBy(invariantId)) {
          if (!varVisited[outputVar]) {
            varVisited[outputVar] = true;
            stack.push_back(outputVar);
          }
        }
      }
    }
  }
}

void BottomUpExplorer::populateCommittedAncestors(Timestamp t) {
  m_committedAncestors.clear();
  m_committedAncestors.reserve(
      m_engine.getDecisionVariables().size());
  
  for (VarIdBase decisionVar : m_engine.getDecisionVariables()) {
    if (m_engine.getTmpTimestamp(decisionVar) == t) {
      m_committedAncestors.push_back(decisionVar);
    }
  }
}

// We expand an invariant by pushing it and its first input variable onto each
// stack.
void BottomUpExplorer::expandInvariantCommit(InvariantId inv) {
  if (invariantIsOnStack.get(inv)) {
    throw DynamicCycleException();
  }
  for (VarIdBase varId : m_engine.getInputVariables(inv)) {
    for (VarIdBase decisionVar : m_committedAncestors) {
      if (m_decisionVarAncestor[varId].find(decisionVar) != m_decisionVarAncestor[varId].end()) {
        pushVariableStack(varId);
        break;
      }
    }
  }
  
  pushInvariantStack(inv);
}

void BottomUpExplorer::expandInvariantProbe(InvariantId inv) {
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

template void BottomUpExplorer::expandInvariant<true>(InvariantId inv);
template void BottomUpExplorer::expandInvariant<false>(InvariantId inv);

template <bool DoCommit>
void BottomUpExplorer::expandInvariant(InvariantId inv) {
  if constexpr(DoCommit) {
    expandInvariantCommit(inv);
  } else {
    expandInvariantProbe(inv);
  }
}

void BottomUpExplorer::notifyCurrentInvariant() {
  m_engine.notifyCurrentDependencyChanged(peekInvariantStack());
}

template bool BottomUpExplorer::visitNextVariable<true>();
template bool BottomUpExplorer::visitNextVariable<false>();

template <bool DoCommit>
bool BottomUpExplorer::visitNextVariable() {
  if constexpr(DoCommit) {
    return true;
  }
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

template void BottomUpExplorer::propagate<true>(Timestamp currentTime);
template void BottomUpExplorer::propagate<false>(Timestamp currentTime);

template <bool DoCommit>
void BottomUpExplorer::propagate(Timestamp currentTime) {
  if constexpr(DoCommit) {
    populateCommittedAncestors(currentTime);
  }
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
        expandInvariant<DoCommit>(m_engine.getDefiningInvariant(currentVar));
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
    bool invariantDone = visitNextVariable<DoCommit>();
    if (invariantDone) {
      if constexpr (DoCommit) {
        m_engine.commitInvariant(peekInvariantStack());
      }
      // The top invariant has finished propagating, so all defined vars can
      // be marked as stable at the current time.
      for (auto defVar : m_engine.getVariablesDefinedBy(peekInvariantStack())) {
        markStable(currentTime, defVar);
        if constexpr (DoCommit) {
          m_engine.commit(defVar);
        }
      }
      popInvariantStack();
    }
  }
}

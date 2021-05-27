
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
      _invariantIsOnStack(expectedSize),
      _inputAncestor(expectedSize) {
  _variableStack.reserve(expectedSize);
  _invariantStack.reserve(expectedSize);
}

void OutputToInputExplorer::populateAncestors() {
  std::vector<bool> varVisited(_engine.getNumVariables() + 1);
  std::deque<IdBase> stack(_engine.getNumVariables() + 1);

  for (IdBase idx = 1; idx <= _engine.getNumVariables(); ++idx) {
    _inputAncestor.register_idx(idx);
    _inputAncestor[idx].clear();
    _inputAncestor[idx].reserve(_engine.getInputVariables().size());
  }

  varVisited.resize(_engine.getNumVariables() + 1);

  for (const VarIdBase inputVar : _engine.getInputVariables()) {
    std::fill(varVisited.begin(), varVisited.end(), NULL_ID);
    stack.clear();
    stack.push_back(inputVar);
    varVisited[inputVar] = true;

    while (stack.size() > 0) {
      const VarIdBase id = stack.back();
      stack.pop_back();
      _inputAncestor[id].emplace(inputVar);

      for (InvariantId invariantId :
           _engine.getListeningInvariants(IdBase(id))) {
        for (VarIdBase outputVar : _engine.getVariablesDefinedBy(invariantId)) {
          if (!varVisited[outputVar]) {
            varVisited[outputVar] = true;
            stack.push_back(outputVar);
          }
        }
      }
    }
  }
}

template bool OutputToInputExplorer::isUpToDate<true>(VarIdBase id);
template bool OutputToInputExplorer::isUpToDate<false>(VarIdBase id);
template <bool OutputToInputMarking>
bool OutputToInputExplorer::isUpToDate(VarIdBase id) {
  if constexpr (OutputToInputMarking) {
    for (const size_t ancestor : _modifiedInputs) {
      if (_inputAncestor.at(id).find(ancestor) != _inputAncestor.at(id).end()) {
        return false;
      }
    }
    return true;
  } else {
    return !_engine.isOnPropagationPath(id);
  }
}

template void OutputToInputExplorer::preprocessVarStack<false>(
    Timestamp currentTimestamp);
template void OutputToInputExplorer::preprocessVarStack<true>(
    Timestamp currentTimestamp);
template <bool OutputToInputMarking>
void OutputToInputExplorer::preprocessVarStack(Timestamp currentTimestamp) {
  size_t newStackSize = 0;
  for (size_t s = 0; s < _varStackIdx; ++s) {
    if (!isUpToDate<OutputToInputMarking>(_variableStack[s])) {
      _variableStack[newStackSize] = _variableStack[s];
      ++newStackSize;
    } else {
      _varIsOnStack.set(_variableStack[s], false);
      markStable(currentTimestamp, _variableStack[s]);
    }
    _varStackIdx = newStackSize;
  }
}

void OutputToInputExplorer::populateModifiedAncestors(Timestamp t) {
  _modifiedInputs.clear();
  _modifiedInputs.reserve(_engine.getInputVariables().size());

  for (VarIdBase inputVar : _engine.getInputVariables()) {
    if (_engine.hasChanged(t, inputVar)) {
      _modifiedInputs.push_back(inputVar);
    }
  }
}

template void OutputToInputExplorer::expandInvariant<true>(InvariantId inv);
template void OutputToInputExplorer::expandInvariant<false>(InvariantId inv);

// We expand an invariant by pushing it and its first input variable onto each
// stack.
template <bool OutputToInputMarking>
void OutputToInputExplorer::expandInvariant(InvariantId inv) {
  if (inv == NULL_ID) {
    return;
  }
  if (_invariantIsOnStack.get(inv)) {
    throw DynamicCycleException();
  }
  VarId nextVar = _engine.getNextParameter(inv);
  while (nextVar != NULL_ID && isUpToDate<OutputToInputMarking>(nextVar)) {
    nextVar = _engine.getNextParameter(inv);
  }

  if (nextVar.id == NULL_ID) {
    return;
  }
  pushVariableStack(nextVar);
  pushInvariantStack(inv);
}

void OutputToInputExplorer::notifyCurrentInvariant() {
  _engine.notifyCurrentParameterChanged(peekInvariantStack());
}

template bool OutputToInputExplorer::visitNextVariable<true>();
template bool OutputToInputExplorer::visitNextVariable<false>();

template <bool OutputToInputMarking>
bool OutputToInputExplorer::visitNextVariable() {
  popVariableStack();
  VarId nextVar = _engine.getNextParameter(peekInvariantStack());
  while (nextVar != NULL_ID && isUpToDate<OutputToInputMarking>(nextVar)) {
    nextVar = _engine.getNextParameter(peekInvariantStack());
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

template void OutputToInputExplorer::propagate<true>(
    Timestamp currentTimestamp);
template void OutputToInputExplorer::propagate<false>(
    Timestamp currentTimestamp);

template <bool OutputToInputMarking>
void OutputToInputExplorer::propagate(Timestamp currentTimestamp) {
  if constexpr (OutputToInputMarking) {
    populateModifiedAncestors(currentTimestamp);
  }
  preprocessVarStack<OutputToInputMarking>(currentTimestamp);
  // recursively expand variables to compute their value.
  while (_varStackIdx > 0) {
    VarId currentVar = peekVariableStack();

    // If the variable is not stable, then expand it.
    if (!isStable(currentTimestamp, currentVar)) {
      // Variable will become stable as it is either not defined or we now
      // expand its invariant. Note that expandInvariant may can sometimes not
      // push a new invariant nor a new variable on the stack, so we must mark
      // the variable as stable before we expand it as this otherwise results in
      // an infinite loop.
      markStable(currentTimestamp, currentVar);
      // Variable must be on
      expandInvariant<OutputToInputMarking>(
          _engine.getDefiningInvariant(currentVar));
      continue;
    }
    if (_invariantStackIdx == 0) {
      popVariableStack();  // we are at an output variable that is already
                           // stable. Just continue!
      continue;
    }
    if (_engine.hasChanged(currentTimestamp, currentVar)) {
      // If the variable is stable and has changed then just send a
      // notification to top invariant (i.e, the one asking for its value)
      notifyCurrentInvariant();
    }
    bool invariantDone = visitNextVariable<OutputToInputMarking>();
    if (invariantDone) {
      // The top invariant has finished propagating, so all defined vars can
      // be marked as stable at the current time.
      for (auto defVar : _engine.getVariablesDefinedBy(peekInvariantStack())) {
        markStable(currentTimestamp, defVar);
      }
      popInvariantStack();
    }
  }
}

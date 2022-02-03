
#include "propagation/outputToInputExplorer.hpp"

#include "core/propagationEngine.hpp"

OutputToInputExplorer::OutputToInputExplorer(PropagationEngine& e,
                                             size_t expectedSize)
    : _engine(e),
      _varStackIdx(0),
      _invariantStackIdx(0),
      _varStableAt(expectedSize),
      _invariantStableAt(expectedSize),
      _invariantIsOnStack(expectedSize),
      _decisionVarAncestor(expectedSize) {
  _variableStack.reserve(expectedSize);
  _invariantStack.reserve(expectedSize);
}

void OutputToInputExplorer::populateAncestors() {
  std::vector<bool> varVisited(_engine.getNumVariables() + 1);
  std::deque<IdBase> stack(_engine.getNumVariables() + 1);

  for (IdBase idx = 1; idx <= _engine.getNumVariables(); ++idx) {
    if (_decisionVarAncestor.size() < idx) {
      _decisionVarAncestor.register_idx(idx);
    }

    _decisionVarAncestor[idx].clear();
  }

  for (const VarIdBase decisionVar : _engine.getDecisionVariables()) {
    std::fill(varVisited.begin(), varVisited.end(), false);
    stack.clear();
    stack.push_back(decisionVar);
    varVisited[decisionVar] = true;

    while (stack.size() > 0) {
      const VarIdBase id = stack.back();
      stack.pop_back();
      _decisionVarAncestor[id].emplace(decisionVar);

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

template bool OutputToInputExplorer::isUpToDate<OutputToInputMarkingMode::NONE>(
    VarIdBase id);
template bool OutputToInputExplorer::isUpToDate<
    OutputToInputMarkingMode::MARK_SWEEP>(VarIdBase id);
template bool OutputToInputExplorer::isUpToDate<
    OutputToInputMarkingMode::TOPOLOGICAL_SORT>(VarIdBase id);
template <OutputToInputMarkingMode MarkingMode>
bool OutputToInputExplorer::isUpToDate(VarIdBase id) {
  if constexpr (MarkingMode == OutputToInputMarkingMode::MARK_SWEEP) {
    for (const size_t ancestor : _engine.getModifiedDecisionVariables()) {
      if (_decisionVarAncestor.at(id).find(ancestor) !=
          _decisionVarAncestor.at(id).end()) {
        return false;
      }
    }
    return true;
  } else if constexpr (MarkingMode ==
                       OutputToInputMarkingMode::TOPOLOGICAL_SORT) {
    return !_engine.isOnPropagationPath(id);
  } else {
    return false;
  }
}

template void OutputToInputExplorer::preprocessVarStack<
    OutputToInputMarkingMode::NONE>(Timestamp currentTimestamp);
template void OutputToInputExplorer::preprocessVarStack<
    OutputToInputMarkingMode::MARK_SWEEP>(Timestamp currentTimestamp);
template void OutputToInputExplorer::preprocessVarStack<
    OutputToInputMarkingMode::TOPOLOGICAL_SORT>(Timestamp currentTimestamp);
template <OutputToInputMarkingMode MarkingMode>
void OutputToInputExplorer::preprocessVarStack(Timestamp currentTimestamp) {
  size_t newStackSize = 0;
  for (size_t s = 0; s < _varStackIdx; ++s) {
    if (!isUpToDate<MarkingMode>(_variableStack[s])) {
      _variableStack[newStackSize] = _variableStack[s];
      ++newStackSize;
    } else {
      markStable(currentTimestamp, _variableStack[s]);
    }
  }
  _varStackIdx = newStackSize;
}

template void OutputToInputExplorer::expandInvariant<
    OutputToInputMarkingMode::NONE>(InvariantId);
template void OutputToInputExplorer::expandInvariant<
    OutputToInputMarkingMode::MARK_SWEEP>(InvariantId);
template void OutputToInputExplorer::expandInvariant<
    OutputToInputMarkingMode::TOPOLOGICAL_SORT>(InvariantId);
// We expand an invariant by pushing it and its first input variable onto each
// stack.
template <OutputToInputMarkingMode MarkingMode>
void OutputToInputExplorer::expandInvariant(InvariantId invariantId) {
  if (invariantId == NULL_ID) {
    return;
  }
  if (_invariantIsOnStack.get(invariantId)) {
    throw DynamicCycleException();
  }
  VarId nextVar = _engine.getNextInput(invariantId);
  while (nextVar != NULL_ID && isUpToDate<MarkingMode>(nextVar)) {
    nextVar = _engine.getNextInput(invariantId);
  }

  if (nextVar.id == NULL_ID) {
    return;
  }
  pushVariableStack(nextVar);
  pushInvariantStack(invariantId);
}

void OutputToInputExplorer::notifyCurrentInvariant() {
  _engine.notifyCurrentInputChanged(peekInvariantStack());
}

template bool
OutputToInputExplorer::pushNextInputVariable<OutputToInputMarkingMode::NONE>();
template bool OutputToInputExplorer::pushNextInputVariable<
    OutputToInputMarkingMode::MARK_SWEEP>();
template bool OutputToInputExplorer::pushNextInputVariable<
    OutputToInputMarkingMode::TOPOLOGICAL_SORT>();

template <OutputToInputMarkingMode MarkingMode>
bool OutputToInputExplorer::pushNextInputVariable() {
  VarId nextVar = _engine.getNextInput(peekInvariantStack());
  while (nextVar != NULL_ID && isUpToDate<MarkingMode>(nextVar)) {
    nextVar = _engine.getNextInput(peekInvariantStack());
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
}

void OutputToInputExplorer::registerInvariant(InvariantId invariantId) {
  _invariantStack.emplace_back(NULL_ID);  // push back just to resize the stack!
  _invariantStableAt.register_idx(invariantId);
  _invariantIsOnStack.register_idx(invariantId, false);
}

template void OutputToInputExplorer::propagate<OutputToInputMarkingMode::NONE>(
    Timestamp currentTimestamp);
template void OutputToInputExplorer::propagate<
    OutputToInputMarkingMode::MARK_SWEEP>(Timestamp currentTimestamp);
template void OutputToInputExplorer::propagate<
    OutputToInputMarkingMode::TOPOLOGICAL_SORT>(Timestamp currentTimestamp);

template <OutputToInputMarkingMode MarkingMode>
void OutputToInputExplorer::propagate(Timestamp currentTimestamp) {
  preprocessVarStack<MarkingMode>(currentTimestamp);
  // recursively expand variables to compute their value.
  while (_varStackIdx > 0) {
    VarId currentVarId = peekVariableStack();

    // If the variable is not stable, then expand it.
    if (!isStable(currentTimestamp, currentVarId)) {
      // Variable will become stable as it is either not defined or we now
      // expand its invariant. Note that expandInvariant may sometimes not
      // push a new invariant nor a new variable on the stack, so we must mark
      // the variable as stable before we expand it as this otherwise results in
      // an infinite loop.
      markStable(currentTimestamp, currentVarId);
      // The variable is upToDate and stable: expand its defining invariant.
      expandInvariant<MarkingMode>(_engine.getDefiningInvariant(currentVarId));
      continue;
    }
    // currentVarId is done: pop it from the stack.
    popVariableStack();
    if (_invariantStackIdx == 0) {
      // we are at an output variable that is already
      // stable. Just continue!
      continue;
    }
    if (_engine.hasChanged(currentTimestamp, currentVarId)) {
      // If the variable is stable and has changed then just send a
      // notification to top invariant (i.e, the one asking for its value)
      notifyCurrentInvariant();
    }
    // push the next input variable of the top invariant
    bool invariantDone = pushNextInputVariable<MarkingMode>();
    if (invariantDone) {
      // The top invariant has finished propagating, so all defined vars can
      // be marked as stable at the current time.
      for (auto defVar : _engine.getVariablesDefinedBy(peekInvariantStack())) {
        markStable(currentTimestamp, defVar);
      }
      popInvariantStack();
    }
  }
  clearRegisteredVariables();
}

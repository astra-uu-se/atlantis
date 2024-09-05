#include "atlantis/propagation/propagation/outputToInputExplorer.hpp"

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::propagation {

OutputToInputExplorer::OutputToInputExplorer(Solver& e, size_t expectedSize)
    : _solver(e),
      _varStackIdx(0),
      _invariantStackIdx(0),
      _varComputedAt(expectedSize),
      _invariantComputedAt(expectedSize),
      _invariantIsOnStack(expectedSize),
      _searchVarAncestors(expectedSize),
      _onPropagationPath(expectedSize),
      _outputToInputMarkingMode(OutputToInputMarkingMode::NONE) {
  _varStack.reserve(expectedSize);
  _invariantStack.reserve(expectedSize);
}

void OutputToInputExplorer::outputToInputStaticMarking() {
  std::vector<bool> varVisited(_solver.numVars());

  for (size_t idx = 0; idx < _solver.numVars(); ++idx) {
    if (_searchVarAncestors.size() <= idx) {
      _searchVarAncestors.register_idx(idx);
    }

    _searchVarAncestors[idx].clear();
  }

  for (const VarId searchVar : _solver.searchVars()) {
    std::fill(varVisited.begin(), varVisited.end(), false);
    std::vector<VarId> stack;
    stack.reserve(_solver.numVars());

    stack.emplace_back(searchVar);
    varVisited[searchVar] = true;

    while (!stack.empty()) {
      const VarId id = stack.back();
      stack.pop_back();
      _searchVarAncestors[id].emplace(searchVar);

      for (const PropagationGraph::ListeningInvariantData& invariantData :
           _solver.listeningInvariantData(id)) {
        for (const VarId outputVar :
             _solver.varsDefinedBy(invariantData.invariantId)) {
          if (!varVisited[outputVar]) {
            varVisited[outputVar] = true;
            stack.push_back(outputVar);
          }
        }
      }
    }
  }
}

void OutputToInputExplorer::inputToOutputExplorationMarking() {
  std::vector<VarId> stack;
  stack.reserve(_solver.numVars());

  _onPropagationPath.assign(_solver.numVars(), false);

  for (const VarId modifiedDecisionVar : _solver.modifiedSearchVar()) {
    stack.emplace_back(modifiedDecisionVar);
    assert(!_onPropagationPath.get(modifiedDecisionVar));
    _onPropagationPath.set(modifiedDecisionVar, true);

    while (!stack.empty()) {
      const VarId id = stack.back();
      stack.pop_back();
      for (const PropagationGraph::ListeningInvariantData& invariantData :
           _solver.listeningInvariantData(id)) {
        for (const VarId outputVar :
             _solver.varsDefinedBy(invariantData.invariantId)) {
          if (!_onPropagationPath.get(outputVar)) {
            _onPropagationPath.set(outputVar, true);
            stack.emplace_back(outputVar);
          }
        }
      }
    }
  }
}

template void OutputToInputExplorer::close<OutputToInputMarkingMode::NONE>();
template void OutputToInputExplorer::close<
    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>();
template void OutputToInputExplorer::close<
    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>();
template <OutputToInputMarkingMode MarkingMode>
void OutputToInputExplorer::close() {
  if constexpr (MarkingMode !=
                OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
    _searchVarAncestors.clear();
  }
  if constexpr (MarkingMode !=
                OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION) {
    _onPropagationPath.clear();
  }
  if constexpr (MarkingMode ==
                OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
    outputToInputStaticMarking();
  }
}

void OutputToInputExplorer::propagate(Timestamp ts) {
  if (_outputToInputMarkingMode == OutputToInputMarkingMode::NONE) {
    propagate<OutputToInputMarkingMode::NONE>(ts);
  } else if (_outputToInputMarkingMode ==
             OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION) {
    inputToOutputExplorationMarking();
    propagate<OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>(ts);
  } else if (_outputToInputMarkingMode ==
             OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
    propagate<OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>(ts);
  }
}

template bool OutputToInputExplorer::isMarked<OutputToInputMarkingMode::NONE>(
    VarId id);
template bool OutputToInputExplorer::isMarked<
    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>(VarId id);
template bool OutputToInputExplorer::isMarked<
    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>(VarId id);
template <OutputToInputMarkingMode MarkingMode>
bool OutputToInputExplorer::isMarked(VarId id) {
  if constexpr (MarkingMode ==
                OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
    return std::any_of(_solver.modifiedSearchVar().begin(),
                       _solver.modifiedSearchVar().end(), [&](size_t ancestor) {
                         return _searchVarAncestors.at(id).contains(ancestor);
                       });
  } else if constexpr (MarkingMode ==
                       OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION) {
    return _onPropagationPath.get(id);
  } else {
    // We should check this with constant expressions
    assert(false);

    // We have no marking: no variable is up-to-date, all variables are all
    // potentially out-of-date and must be recomputed.
    return true;
  }
}

template void OutputToInputExplorer::preprocessVarStack<
    OutputToInputMarkingMode::NONE>(Timestamp currentTimestamp);
template void OutputToInputExplorer::preprocessVarStack<
    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>(
    Timestamp currentTimestamp);
template void OutputToInputExplorer::preprocessVarStack<
    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>(
    Timestamp currentTimestamp);
template <OutputToInputMarkingMode MarkingMode>
void OutputToInputExplorer::preprocessVarStack(Timestamp currentTimestamp) {
  size_t newStackSize = 0;
  for (size_t s = 0; s < _varStackIdx; ++s) {
    if constexpr (MarkingMode == OutputToInputMarkingMode::NONE) {
      _varStack[newStackSize] = _varStack[s];
      ++newStackSize;
    } else {
      if (isMarked<MarkingMode>(_varStack[s])) {
        _varStack[newStackSize] = _varStack[s];
        ++newStackSize;
      } else {
        setComputed(currentTimestamp, _varStack[s]);
      }
    }
  }
  _varStackIdx = newStackSize;
}

template void OutputToInputExplorer::expandInvariant<
    OutputToInputMarkingMode::NONE>(InvariantId);
template void OutputToInputExplorer::expandInvariant<
    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>(InvariantId);
template void OutputToInputExplorer::expandInvariant<
    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>(InvariantId);
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
  // nextInput gets the source if the input would be a view:
  VarId nextVar = _solver.nextInput(invariantId);
  if constexpr (MarkingMode != OutputToInputMarkingMode::NONE) {
    while (nextVar != NULL_ID && !isMarked<MarkingMode>(nextVar)) {
      nextVar = _solver.nextInput(invariantId);
    }
  }
  if (nextVar == NULL_ID) {
    return;
  }
  pushVarStack(nextVar);
  pushInvariantStack(invariantId);
}

void OutputToInputExplorer::notifyCurrentInvariant() {
  _solver.notifyCurrentInputChanged(peekInvariantStack());
}

template bool
OutputToInputExplorer::pushNextInputVar<OutputToInputMarkingMode::NONE>();
template bool OutputToInputExplorer::pushNextInputVar<
    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>();
template bool OutputToInputExplorer::pushNextInputVar<
    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>();

template <OutputToInputMarkingMode MarkingMode>
bool OutputToInputExplorer::pushNextInputVar() {
  VarId nextVar = _solver.nextInput(peekInvariantStack());
  if constexpr (MarkingMode != OutputToInputMarkingMode::NONE) {
    while (nextVar != NULL_ID && !isMarked<MarkingMode>(nextVar)) {
      nextVar = _solver.nextInput(peekInvariantStack());
    }
  }
  if (nextVar == NULL_ID) {
    return true;  // done with invariant
  }
  pushVarStack(nextVar);
  return false;  // invariant has more input variables
}

void OutputToInputExplorer::registerVar(VarId id) {
  _varStack.emplace_back(NULL_ID);  // push back just to resize the stack!
  _varComputedAt.register_idx(id, NULL_TIMESTAMP);
}

void OutputToInputExplorer::registerInvariant(InvariantId invariantId) {
  _invariantStack.emplace_back(NULL_ID);  // push back just to resize the stack!
  _invariantComputedAt.register_idx(invariantId, NULL_TIMESTAMP);
  _invariantIsOnStack.register_idx(invariantId, false);
}

template void OutputToInputExplorer::propagate<OutputToInputMarkingMode::NONE>(
    Timestamp currentTimestamp);
template void OutputToInputExplorer::propagate<
    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>(
    Timestamp currentTimestamp);
template void OutputToInputExplorer::propagate<
    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>(
    Timestamp currentTimestamp);

template <OutputToInputMarkingMode MarkingMode>
void OutputToInputExplorer::propagate(Timestamp currentTimestamp) {
  preprocessVarStack<MarkingMode>(currentTimestamp);
  // recursively expand variables to compute their value.
  while (_varStackIdx > 0) {
    const VarId currentVarId = peekVarStack();

    // If the variable is not computed, then expand it.
    if (!isComputed(currentTimestamp, currentVarId)) {
      // Variable will become computed as it is either not defined or we now
      // expand its invariant. Note that expandInvariant may sometimes not
      // push a new invariant nor a new variable on the stack, so we must mark
      // the variable as computed before we expand it as this otherwise
      // results in an infinite loop.
      setComputed(currentTimestamp, currentVarId);
      // The variable is marked and computed: expand its defining invariant.
      expandInvariant<MarkingMode>(_solver.definingInvariant(currentVarId));
      continue;
    }
    // currentVarId is done: pop it from the stack.
    popVarStack();
    if (_invariantStackIdx == 0) {
      // we are at an output variable that is already computed. Just continue!
      continue;
    }
    if (_solver.hasChanged(currentTimestamp, currentVarId)) {
      // If the variable is computed and has changed then just send a
      // notification to top invariant (i.e, the invariant the variable is an
      // input to)
      notifyCurrentInvariant();
    }
    // push the next input variable of the top invariant
    // returns false if there are no more variables to push
    if (pushNextInputVar<MarkingMode>()) {
      // The top invariant has finished propagating, so all defined vars can
      // be marked as compte at the current time.
      for (const VarId defVar : _solver.varsDefinedBy(peekInvariantStack())) {
        setComputed(currentTimestamp, defVar);
      }
      popInvariantStack();
    }
  }
  clearRegisteredVars();
}
}  // namespace atlantis::propagation

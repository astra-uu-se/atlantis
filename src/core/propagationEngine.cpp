#include "core/propagationEngine.hpp"

#include <iostream>

PropagationEngine::PropagationEngine()
    : _propagationMode(PropagationMode::INPUT_TO_OUTPUT),
      _numVariables(0),
      _propGraph(ESTIMATED_NUM_OBJECTS),
      _outputToInputExplorer(*this, ESTIMATED_NUM_OBJECTS),
      _isEnqueued(ESTIMATED_NUM_OBJECTS),
      _modifiedSearchVariables(),
      _searchVariablesModifiedAt(NULL_TIMESTAMP) {}

PropagationGraph& PropagationEngine::propGraph() { return _propGraph; }

void PropagationEngine::open() {
  if (_isOpen) {
    throw EngineOpenException("Engine already open.");
  }
  if (_engineState != EngineState::IDLE) {
    throw EngineStateException("Engine must be idle before opening.");
  }
  _isOpen = true;
}

void PropagationEngine::close() {
  if (!_isOpen) {
    throw EngineClosedException("Engine already closed.");
  }

  ++_currentTimestamp;  // todo: Is it safe to increment time here? What if a
                        // user tried to change a variable but without a begin
                        // move? But we should ignore it anyway then...
  _isOpen = false;
  try {
    _propGraph.close();
  } catch (std::exception const& e) {
    std::cout << "foo";
  }
  if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
    if (outputToInputMarkingMode() == OutputToInputMarkingMode::NONE) {
      _outputToInputExplorer.close<OutputToInputMarkingMode::NONE>();
    } else if (outputToInputMarkingMode() ==
               OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
      _outputToInputExplorer
          .close<OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>();
    } else if (outputToInputMarkingMode() ==
               OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION) {
      _outputToInputExplorer
          .close<OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>();
    }
  }

#ifndef NDEBUG
  for (const VarIdBase varId : searchVariables()) {
    // Assert that if search variable varId is modified,
    // then it is in the set of modified search variables
    assert(_store.intVar(varId).hasChanged(_currentTimestamp) ==
           (_modifiedSearchVariables.find(varId) !=
            _modifiedSearchVariables.end()));
  }
#endif

  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();

#ifndef NDEBUG
  for (const size_t varId : _modifiedSearchVariables) {
    // assert that decsion variable varId is no longer modified.
    assert(!_store.intVar(varId).hasChanged(_currentTimestamp));
  }
#endif
}

//---------------------Registration---------------------
void PropagationEngine::enqueueComputedVar(Timestamp, VarId id) {
  // logDebug("\t\t\tMaybe changed: " << _store.intVar(id));
  if (_isEnqueued.get(id)) {
    // logDebug("\t\t\talready enqueued");
    return;
  }
  // logDebug("\t\t\tpushed on stack");
  _propGraph.enqueuePropagationQueue(id);
  _isEnqueued.set(id, true);
}

void PropagationEngine::registerInvariantInput(InvariantId invariantId,
                                               VarId inputId, LocalId localId) {
  assert(localId < _store.invariant(invariantId).notifiableVarsSize());
  const auto id = sourceId(inputId);
  _propGraph.registerInvariantInput(invariantId, id);
  _listeningInvariantData[id].emplace_back(
      ListeningInvariantData{invariantId, localId});
}

void PropagationEngine::registerDefinedVariable(VarId varId,
                                                InvariantId invariantId) {
  _propGraph.registerDefinedVariable(sourceId(varId), invariantId);
}

void PropagationEngine::registerVar(VarId id) {
  _numVariables++;
  _propGraph.registerVar(id);
  _outputToInputExplorer.registerVar(id);
  _isEnqueued.register_idx(id, false);
}

void PropagationEngine::registerInvariant(InvariantId invariantId) {
  _propGraph.registerInvariant(invariantId);
  _outputToInputExplorer.registerInvariant(invariantId);
}

//---------------------Propagation---------------------

VarId PropagationEngine::dequeueComputedVar(Timestamp) {
  assert(propagationMode() == PropagationMode::INPUT_TO_OUTPUT ||
         _engineState == EngineState::COMMIT);
  if (_propGraph.propagationQueueEmpty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar(_propGraph.dequeuePropagationQueue());
  _isEnqueued.set(nextVar, false);
  // Due to enqueueComputedVar, all variables in the queue are "active".
  return nextVar;
}

void PropagationEngine::clearPropagationQueue() {
  _propGraph.clearPropagationQueue();
  _isEnqueued.assign_all(false);
}

void PropagationEngine::recomputeAndCommit() {
  // TODO: This is a very inefficient way of initialising!
  size_t tries = 0;
  bool done = false;
  while (!done) {
    done = true;
    if (tries++ > _store.numVariables()) {
      throw FailedToInitialise();
    }
    for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
         ++iter) {
      assert((*iter) != nullptr);
      (*iter)->recompute(_currentTimestamp, *this);
    }
    for (auto iter = _store.intVarBegin(); iter != _store.intVarEnd(); ++iter) {
      if (iter->hasChanged(_currentTimestamp)) {
        done = false;
        iter->commit();
      }
    }
  }
  // We must commit all invariants once everything is stable.
  // Commiting an invariant will commit any internal datastructure.
  for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
       ++iter) {
    (*iter)->commit(_currentTimestamp, *this);
  }
  clearPropagationQueue();
}

//--------------------- Move semantics ---------------------
void PropagationEngine::beginMove() {
  assert(!_isOpen);
  assert(_engineState == EngineState::IDLE);

  ++_currentTimestamp;
  _engineState = EngineState::MOVE;
}

void PropagationEngine::endMove() {
  assert(_engineState == EngineState::MOVE);
  _engineState = EngineState::IDLE;
}

void PropagationEngine::beginProbe() {
  assert(!_isOpen);
  assert(_engineState == EngineState::IDLE);
  _engineState = EngineState::PROBE;
}

void PropagationEngine::query(VarId id) {
  assert(!_isOpen);
  assert(_engineState != EngineState::IDLE &&
         _engineState != EngineState::PROCESSING);

  if (_propagationMode != PropagationMode::INPUT_TO_OUTPUT) {
    _outputToInputExplorer.registerForPropagation(_currentTimestamp,
                                                  sourceId(id));
  }
}

void PropagationEngine::endProbe() {
  assert(_engineState == EngineState::PROBE);

  _engineState = EngineState::PROCESSING;
  try {
    if (_propagationMode == PropagationMode::INPUT_TO_OUTPUT) {
      propagate<false>();
    } else {
      outputToInputPropagate();
    }
    _engineState = EngineState::IDLE;
  } catch (std::exception const& e) {
    _engineState = EngineState::IDLE;
    throw e;
  }
}

void PropagationEngine::beginCommit() {
  assert(!_isOpen);
  assert(_engineState == EngineState::IDLE);

  _outputToInputExplorer.clearRegisteredVariables();

  _engineState = EngineState::COMMIT;
}

void PropagationEngine::endCommit() {
  assert(_engineState == EngineState::COMMIT);

  _engineState = EngineState::PROCESSING;

  try {
#ifndef NDEBUG
    if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT &&
        outputToInputMarkingMode() ==
            OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
      for (VarIdBase varId : searchVariables()) {
        // Assert that if decision variable varId is modified,
        // then it is in the set of modified decision variables
        assert(_store.intVar(varId).hasChanged(_currentTimestamp) ==
               (_modifiedSearchVariables.find(varId) !=
                _modifiedSearchVariables.end()));
      }
    }
#endif

    propagate<true>();

#ifndef NDEBUG
    if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT &&
        outputToInputMarkingMode() ==
            OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
      for (size_t varId : _modifiedSearchVariables) {
        // assert that decsion variable varId is no longer modified.
        assert(!_store.intVar(varId).hasChanged(_currentTimestamp));
      }
    }
#endif
    _engineState = EngineState::IDLE;
  } catch (std::exception const& e) {
    _engineState = EngineState::IDLE;
    throw e;
  }
}

template void PropagationEngine::propagate<true>();
template void PropagationEngine::propagate<false>();

// Propagates at the current internal timestamp of the engine.
template <bool DoCommit>
void PropagationEngine::propagate() {
// #define PROPAGATION_DEBUG
// #define PROPAGATION_DEBUG_COUNTING
#ifdef PROPAGATION_DEBUG
  setLogLevel(debug);
  logDebug("Starting propagation");
#endif
#ifdef PROPAGATION_DEBUG_COUNTING
  std::vector<std::unordered_map<size_t, Int>> notificationCount(
      _store.numInvariants());
#endif

  for (VarId queuedVar = dequeueComputedVar(_currentTimestamp);
       queuedVar.id != NULL_ID;
       queuedVar = dequeueComputedVar(_currentTimestamp)) {
    // queuedVar has been computed under _currentTimestamp
    const IntVar& variable = _store.intVar(queuedVar);

    const InvariantId definingInvariant =
        _propGraph.definingInvariant(queuedVar);

#ifdef PROPAGATION_DEBUG
    logDebug("\tPropagating " << variable);
    logDebug("\t\tDepends on invariant: " << definingInvariant);
#endif

    if (definingInvariant != NULL_ID) {
      Invariant& defInv = _store.invariant(definingInvariant);
      if (queuedVar == defInv.primaryDefinedVar()) {
        const Int oldValue = variable.value(_currentTimestamp);
        defInv.compute(_currentTimestamp, *this);
        for (const VarId inputId : defInv.nonPrimaryDefinedVars()) {
          if (hasChanged(_currentTimestamp, inputId)) {
            enqueueComputedVar(_currentTimestamp, inputId);
          }
        }
        if (oldValue == variable.value(_currentTimestamp)) {
#ifdef PROPAGATION_DEBUG
          logDebug("\t\tVariable did not change after compute: ignoring.");
#endif
          continue;
        }
        if constexpr (DoCommit) {
          defInv.commit(_currentTimestamp, *this);
        }
      }
    }

    if constexpr (DoCommit) {
      commitIf(_currentTimestamp, queuedVar);
    }

    for (const auto& toNotify : _listeningInvariantData[queuedVar]) {
      Invariant& invariant = _store.invariant(toNotify.invariantId);

#ifdef PROPAGATION_DEBUG
      logDebug("\t\tNotifying invariant:" << toNotify.invariantId
                                          << " with localId: "
                                          << toNotify.localId);
#endif
#ifdef PROPAGATION_DEBUG_COUNTING
      notificationCount.at(toNotify.invariantId.id - 1)[variable.id().id] =
          notificationCount.at(toNotify.invariantId.id - 1)[variable.id().id] +
          1;
#endif

      invariant.notify(toNotify.localId);
      enqueueComputedVar(_currentTimestamp, invariant.primaryDefinedVar());
    }
  }

#ifdef PROPAGATION_DEBUG_COUNTING
  logDebug("Printing notification counts");
  for (int i = 0; i < notificationCount.size(); ++i) {
    logDebug("\tInvariant " << i + 1);
    for (const auto [k, v] : notificationCount.at(i)) {
      logDebug("\t\tVarId(" << k << "): " << v);
    }
  }
#endif
#ifdef PROPAGATION_DEBUG
  logDebug("Propagation done\n");
#endif
}
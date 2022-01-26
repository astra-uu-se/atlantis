#include "core/propagationEngine.hpp"

#include <iostream>

PropagationEngine::PropagationEngine()
    : _propagationMode(PropagationMode::INPUT_TO_OUTPUT),
      _numVariables(0),
      _propGraph(ESTIMATED_NUM_OBJECTS),
      _outputToInputExplorer(*this, ESTIMATED_NUM_OBJECTS),
      _isEnqueued(ESTIMATED_NUM_OBJECTS),
      _varIsOnPropagationPath(ESTIMATED_NUM_OBJECTS),
      _propagationPathQueue(),
      _modifiedDecisionVariables(),
      _decisionVariablesModifiedAt(NULL_TIMESTAMP) {}

PropagationGraph& PropagationEngine::getPropGraph() { return _propGraph; }

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
    _outputToInputExplorer.populateAncestors();
  }

#ifndef NDEBUG
  for (VarIdBase varId : getDecisionVariables()) {
    // Assert that if decision variable varId is modified,
    // then it is in the set of modified decision variables
    assert(_store.getIntVar(varId).hasChanged(_currentTimestamp) ==
           (_modifiedDecisionVariables.find(varId) !=
            _modifiedDecisionVariables.end()));
  }
#endif

  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();

#ifndef NDEBUG
  for (size_t varId : _modifiedDecisionVariables) {
    // assert that decsion variable varId is no longer modified.
    assert(!_store.getIntVar(varId).hasChanged(_currentTimestamp));
  }
#endif
}

//---------------------Registration---------------------
void PropagationEngine::notifyMaybeChanged(Timestamp, VarId id) {
  // logDebug("\t\t\tMaybe changed: " << _store.getIntVar(id));
  if (_isEnqueued.get(id)) {
    // logDebug("\t\t\talready enqueued");
    return;
  }
  // logDebug("\t\t\tpushed on stack");
  _propGraph._propagationQueue.push(id);
  _isEnqueued.set(id, true);
}

void PropagationEngine::queueForPropagation(Timestamp, VarId id) {
  // logDebug("\t\t\tMaybe changed: " << _store.getIntVar(id));
  if (_isEnqueued.get(id)) {
    // logDebug("\t\t\talready enqueued");
    return;
  }
  // logDebug("\t\t\tpushed on stack");
  _propGraph._propagationQueue.push(id);
  _isEnqueued.set(id, true);
}

void PropagationEngine::registerInvariantInput(InvariantId invariantId,
                                               VarId inputId, LocalId localId) {
  assert(localId < _store.getInvariant(invariantId).notifiableVarsSize());
  auto sourceId = getSourceId(inputId);
  _propGraph.registerInvariantInput(invariantId, sourceId);
  _listeningInvariantData[sourceId].emplace_back(
      ListeningInvariantData{invariantId, localId});
}

void PropagationEngine::registerDefinedVariable(VarId varId,
                                                InvariantId invariantId) {
  _propGraph.registerDefinedVariable(getSourceId(varId), invariantId);
}

void PropagationEngine::registerVar(VarId id) {
  _numVariables++;
  _propGraph.registerVar(id);
  _outputToInputExplorer.registerVar(id);
  _isEnqueued.register_idx(id, false);
  _varIsOnPropagationPath.register_idx(id, false);
}

void PropagationEngine::registerInvariant(InvariantId invariantId) {
  _propGraph.registerInvariant(invariantId);
  _outputToInputExplorer.registerInvariant(invariantId);
}

//---------------------Propagation---------------------

VarId PropagationEngine::getNextStableVariable(Timestamp) {
  if (_propGraph._propagationQueue.empty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar(_propGraph._propagationQueue.top());
  _propGraph._propagationQueue.pop();
  _isEnqueued.set(nextVar, false);
  // Due to notifyMaybeChanged, all variables in the queue are "active".
  return nextVar;
}

void PropagationEngine::clearPropagationQueue() {
  while (!_propGraph._propagationQueue.empty()) {
    _propGraph._propagationQueue.pop();
  }
  _isEnqueued.assign_all(false);
}

void PropagationEngine::recomputeAndCommit() {
  // TODO: This is a very inefficient way of initialising!
  size_t tries = 0;
  bool done = false;
  while (!done) {
    done = true;
    if (tries++ > _store.getNumVariables()) {
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
  if (_propagationMode == PropagationMode::MIXED) {
    clearPropagationPath();
  }

  _engineState = EngineState::MOVE;
}

void PropagationEngine::endMove() {
  assert(_engineState == EngineState::MOVE);
  _engineState = EngineState::IDLE;
}

void PropagationEngine::beginQuery() {
  assert(!_isOpen);
  assert(_engineState == EngineState::IDLE);
  _engineState = EngineState::QUERY;
}

void PropagationEngine::query(VarId id) {
  assert(!_isOpen);
  assert(_engineState != EngineState::IDLE &&
         _engineState != EngineState::PROCESSING);

  if (_propagationMode != PropagationMode::INPUT_TO_OUTPUT) {
    _outputToInputExplorer.registerForPropagation(_currentTimestamp,
                                                  getSourceId(id));
  }
}

void PropagationEngine::endQuery() {
  assert(_engineState == EngineState::QUERY);

  _engineState = EngineState::PROCESSING;
  try {
    switch (_propagationMode) {
      case PropagationMode::INPUT_TO_OUTPUT:
        propagate<false>();
        break;
      case PropagationMode::OUTPUT_TO_INPUT:
        outputToInputPropagate<true>();
        break;
      case PropagationMode::MIXED:
        outputToInputPropagate<false>();
        break;
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
    if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
      for (VarIdBase varId : getDecisionVariables()) {
        // Assert that if decision variable varId is modified,
        // then it is in the set of modified decision variables
        assert(_store.getIntVar(varId).hasChanged(_currentTimestamp) ==
               (_modifiedDecisionVariables.find(varId) !=
                _modifiedDecisionVariables.end()));
      }
    }
#endif

    propagate<true>();

#ifndef NDEBUG
    if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
      for (size_t varId : _modifiedDecisionVariables) {
        // assert that decsion variable varId is no longer modified.
        assert(!_store.getIntVar(varId).hasChanged(_currentTimestamp));
      }
    }
#endif
    _engineState = EngineState::IDLE;
  } catch (std::exception const& e) {
    _engineState = EngineState::IDLE;
    throw e;
  }
}

void PropagationEngine::markPropagationPathAndClearPropagationQueue() {
  // We cannot iterate over a priority_queue so we cannot copy it.
  // TODO: replace priority_queue of _propGraph._propagationQueue with custom
  // queue.
  while (!_propGraph._propagationQueue.empty()) {
    auto id = _propGraph._propagationQueue.top();
    _isEnqueued.set(id, false);
    _propagationPathQueue.push(id);
    _propGraph._propagationQueue.pop();
  }

  while (!_propagationPathQueue.empty()) {
    VarId currentVar = _propagationPathQueue.front();
    _propagationPathQueue.pop();
    if (_varIsOnPropagationPath.get(currentVar)) {
      continue;
    }
    _varIsOnPropagationPath.set(currentVar, true);
    for (auto& listeningInv : _listeningInvariantData.at(currentVar)) {
      for (VarIdBase definedVar :
           _propGraph.getVariablesDefinedBy(listeningInv.invariantId)) {
        if (!_varIsOnPropagationPath.get(definedVar)) {
          _propagationPathQueue.push(definedVar);
        }
      }
    }
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
      _store.getNumInvariants());
#endif

  for (VarId stableVarId = getNextStableVariable(_currentTimestamp);
       stableVarId.id != NULL_ID;
       stableVarId = getNextStableVariable(_currentTimestamp)) {
    IntVar& variable = _store.getIntVar(stableVarId);

    InvariantId definingInvariant =
        _propGraph.getDefiningInvariant(stableVarId);

#ifdef PROPAGATION_DEBUG
    logDebug("\tPropagating " << variable);
    logDebug("\t\tDepends on invariant: " << definingInvariant);
#endif

    if (definingInvariant != NULL_ID) {
      Invariant& defInv = _store.getInvariant(definingInvariant);
      if (stableVarId == defInv.getPrimaryDefinedVar()) {
        Int oldValue = variable.getValue(_currentTimestamp);
        defInv.compute(_currentTimestamp, *this);
        defInv.queueNonPrimaryDefinedVarsForPropagation(_currentTimestamp,
                                                        *this);
        if (oldValue == variable.getValue(_currentTimestamp)) {
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
      commitIf(_currentTimestamp, stableVarId);
    }

    for (auto& toNotify : _listeningInvariantData[stableVarId]) {
      Invariant& invariant = _store.getInvariant(toNotify.invariantId);

#ifdef PROPAGATION_DEBUG
      logDebug("\t\tNotifying invariant:" << toNotify.invariantId
                                          << " with localId: "
                                          << toNotify.localId);
#endif
#ifdef PROPAGATION_DEBUG_COUNTING
      notificationCount.at(toNotify.invariantId.id - 1)[variable.getId().id] =
          notificationCount.at(toNotify.invariantId.id -
                               1)[variable.getId().id] +
          1;
#endif

      invariant.notify(toNotify.localId);
      queueForPropagation(_currentTimestamp, invariant.getPrimaryDefinedVar());
    }
  }

#ifdef PROPAGATION_DEBUG_COUNTING
  logDebug("Printing notification counts");
  for (int i = 0; i < notificationCount.size(); ++i) {
    logDebug("\tInvariant " << i + 1);
    for (auto [k, v] : notificationCount.at(i)) {
      logDebug("\t\tVarId(" << k << "): " << v);
    }
  }
#endif
#ifdef PROPAGATION_DEBUG
  logDebug("Propagation done\n");
#endif
}
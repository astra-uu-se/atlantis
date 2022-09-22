#include "core/propagationEngine.hpp"

PropagationEngine::PropagationEngine()
    : _propagationMode(PropagationMode::INPUT_TO_OUTPUT),
      _propGraph(_store, ESTIMATED_NUM_OBJECTS),
      _outputToInputExplorer(*this, ESTIMATED_NUM_OBJECTS),
      _isEnqueued(ESTIMATED_NUM_OBJECTS),
      _modifiedSearchVariables() {}

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

  incCurrentTimestamp();

  _isOpen = false;
  try {
    _propGraph.close(currentTimestamp());
  } catch (std::exception const& e) {
    std::cout << "foo";
  }

  if (_propGraph.numLayers() > 1) {
    _layerQueueIndex.assign(_propGraph.numLayers(), 0);
    _layerQueue.resize(_propGraph.numLayers(), std::vector<VarIdBase>{});
    for (size_t layer = 1; layer < _propGraph.numLayers(); ++layer) {
      _layerQueue[layer].resize(_propGraph.numVariablesInLayer(layer));
    }
  } else {
    _layerQueueIndex.clear();
    _layerQueue.clear();
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

  // Assert that if search variable varId is modified,
  // then it is in the set of modified search variables
  assert(std::all_of(searchVariables().begin(), searchVariables().end(),
                     [&](const VarIdBase varId) {
                       return _store.intVar(varId).hasChanged(
                                  _currentTimestamp) ==
                              _modifiedSearchVariables.contains(varId);
                     }));
  assert(std::all_of(searchVariables().begin(), searchVariables().end(),
                     [&](const VarIdBase varId) {
                       return _store.intVar(varId).hasChanged(
                                  _currentTimestamp) ==
                              _modifiedSearchVariables.contains(varId);
                     }));

  // close all invariants
  closeInvariants();

  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();

  // assert that decsion variable varId is no longer modified.
  assert(std::all_of(_modifiedSearchVariables.begin(),
                     _modifiedSearchVariables.end(), [&](const size_t varId) {
                       return !_store.intVar(varId).hasChanged(
                           _currentTimestamp);
                     }));
}

//---------------------Registration---------------------
void PropagationEngine::enqueueComputedVar(VarId id) {
  if (_isEnqueued.get(id)) {
    return;
  }
  _propGraph.enqueuePropagationQueue(id);
  _isEnqueued.set(id, true);
}

void PropagationEngine::enqueueComputedVar(VarId id, size_t curLayer) {
  if (_isEnqueued.get(id)) {
    return;
  }
  const size_t varLayer = _propGraph.layer(id);
  if (varLayer == curLayer) {
    _propGraph.enqueuePropagationQueue(id);
  } else {
    _layerQueue[varLayer][_layerQueueIndex[varLayer]] = id;
    ++_layerQueueIndex[varLayer];
  }
  _isEnqueued.set(id, true);
}

void PropagationEngine::registerInvariantInput(InvariantId invariantId,
                                               VarId inputId, LocalId localId,
                                               bool isDynamicInput) {
  assert(localId < _store.invariant(invariantId).notifiableVarsSize());
  const auto id = sourceId(inputId);
  _propGraph.registerInvariantInput(invariantId, id, isDynamicInput);
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

void PropagationEngine::closeInvariants() {
  for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
       ++iter) {
    (*iter)->close(_currentTimestamp, *this);
  }
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

//--------------------- Propagation ---------------------
void PropagationEngine::beginMove() {
  assert(!_isOpen);
  assert(_engineState == EngineState::IDLE);

  incCurrentTimestamp();
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
      if (_propGraph.numLayers() == 1) {
        propagate<false, true>();
      } else {
        propagate<false, false>();
      }
    } else {
      // Assert that if decision variable varId is modified,
      // then it is in the set of modified decision variables
      assert(outputToInputMarkingMode() !=
                 OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC ||
             std::all_of(searchVariables().begin(), searchVariables().end(),
                         [&](const VarIdBase varId) {
                           return _store.intVar(varId).hasChanged(
                                      _currentTimestamp) ==
                                  _modifiedSearchVariables.contains(varId);
                         }));
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
    // Assert that if decision variable varId is modified,
    // then it is in the set of modified decision variables
    assert(_propagationMode != PropagationMode::OUTPUT_TO_INPUT ||
           outputToInputMarkingMode() !=
               OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC ||
           std::all_of(searchVariables().begin(), searchVariables().end(),
                       [&](const VarIdBase varId) {
                         return _store.intVar(varId).hasChanged(
                                    _currentTimestamp) ==
                                _modifiedSearchVariables.contains(varId);
                       }));
    if (_propGraph.numLayers() == 1) {
      propagate<true, true>();
    } else {
      propagate<true, false>();
    }

    // assert that decsion variable varId is no longer modified.
    assert(_propagationMode != PropagationMode::OUTPUT_TO_INPUT ||
           std::all_of(_modifiedSearchVariables.begin(),
                       _modifiedSearchVariables.end(), [&](const size_t varId) {
                         return !_store.intVar(varId).hasChanged(
                             _currentTimestamp);
                       }));
    _engineState = EngineState::IDLE;
  } catch (std::exception const& e) {
    _engineState = EngineState::IDLE;
    throw e;
  }
}

template void PropagationEngine::propagate<false, false>();
template void PropagationEngine::propagate<false, true>();
template void PropagationEngine::propagate<true, false>();
template void PropagationEngine::propagate<true, true>();
// Propagates at the current internal timestamp of the engine.
template <bool DoCommit, bool SingleLayer>
void PropagationEngine::propagate() {
  size_t curLayer = 0;
  while (true) {
    for (VarId queuedVar = dequeueComputedVar(_currentTimestamp);
         queuedVar.id != NULL_ID;
         queuedVar = dequeueComputedVar(_currentTimestamp)) {
      assert(queuedVar.idType == VarIdType::var);
      assert(_propGraph.layer(queuedVar) == curLayer);
      // queuedVar has been computed under _currentTimestamp
      const IntVar& variable = _store.intVar(queuedVar);

      const InvariantId definingInvariant =
          _propGraph.definingInvariant(queuedVar);

      if (definingInvariant != NULL_ID) {
        Invariant& defInv = _store.invariant(definingInvariant);
        if (queuedVar == defInv.primaryDefinedVar()) {
          const Int oldValue = variable.value(_currentTimestamp);
          defInv.compute(_currentTimestamp, *this);
          for (const VarId inputId : defInv.nonPrimaryDefinedVars()) {
            if (hasChanged(_currentTimestamp, inputId)) {
              assert(!_isEnqueued.get(inputId));
              _propGraph.enqueuePropagationQueue(inputId);
              _isEnqueued.set(inputId, true);
            }
          }
          if constexpr (DoCommit) {
            defInv.commit(_currentTimestamp, *this);
          }
          if (oldValue == variable.value(_currentTimestamp)) {
            continue;
          }
        }
      }

      if constexpr (DoCommit) {
        commitIf(_currentTimestamp, queuedVar);
      }

      for (const auto& toNotify : _listeningInvariantData[queuedVar]) {
        Invariant& invariant = _store.invariant(toNotify.invariantId);
        invariant.notify(toNotify.localId);
        assert(invariant.primaryDefinedVar() != NULL_ID);
        assert(invariant.primaryDefinedVar().idType == VarIdType::var);
        if constexpr (SingleLayer) {
          assert(_propGraph.layer(invariant.primaryDefinedVar()) == 0);
          enqueueComputedVar(invariant.primaryDefinedVar());
        } else {
          enqueueComputedVar(invariant.primaryDefinedVar(), curLayer);
        }
      }
    }
    // Done with propagating current layer.
    if constexpr (SingleLayer) {
      return;
    } else {
      // Find next layer that has queued variables:
      while (++curLayer < _propGraph.numLayers() &&
             _layerQueueIndex[curLayer] == 0) {
        ;
      }

      if (curLayer >= _propGraph.numLayers()) {
        // All layers have been propogated
        assert(_layerQueueIndex.size() == _propGraph.numLayers());
        assert(std::all_of(_layerQueueIndex.begin(), _layerQueueIndex.end(),
                           [&](const Timestamp lqi) { return lqi == 0; }));
        return;
      }
      // There are variables to enqueue for the new layer:
      assert(_layerQueueIndex[curLayer] > 0);
      // Topologically order the new layer if necessary:
      if (_propGraph.hasDynamicCycle(curLayer)) {
        _propGraph.topologicallyOrder(_currentTimestamp, curLayer);
      }
      // Add all queued variables to the propagation queue:
      while (_layerQueueIndex[curLayer] > 0) {
        _propGraph.enqueuePropagationQueue(
            _layerQueue[curLayer][--_layerQueueIndex[curLayer]]);
      }
      assert(_layerQueueIndex[curLayer] == 0);
    }
  }
}

void PropagationEngine::computeBounds() {
  IdMap<InvariantId, Int> inputsToCompute(numInvariants());

  for (size_t invariantId = 1u; invariantId <= numInvariants(); ++invariantId) {
    inputsToCompute.register_idx(invariantId,
                                 inputVariables(invariantId).size());
  }

  // Search variables might now have been computed yet
  for (size_t varId = 1u; varId <= numVariables(); ++varId) {
    if (definingInvariant(varId) == NULL_ID) {
      for (const InvariantId listeningInvariantId :
           listeningInvariants(varId)) {
        --inputsToCompute[listeningInvariantId];
      }
    }
  }

  auto cmp = [&](InvariantId a, InvariantId b) {
    if (inputsToCompute[a] == inputsToCompute[b]) {
      return a < b;
    }
    return inputsToCompute[a] < inputsToCompute[b];
  };

  std::set<InvariantId, decltype(cmp)> invariantQueue(cmp);

  for (size_t invariantId = 1; invariantId <= numInvariants(); ++invariantId) {
    invariantQueue.emplace(invariantId);
  }

  while (!invariantQueue.empty()) {
    const InvariantId invariantId = *invariantQueue.begin();
    assert(inputsToCompute[invariantId] >= 0);

    invariantQueue.erase(invariantId);
    assert(!invariantQueue.contains(invariantId));

    assert(!invariantQueue.contains(invariantId));
    // If the following assertion fails, then inputsToCompute[i] was
    // updated before removing invariant i:
    assert(std::all_of(
        invariantQueue.begin(), invariantQueue.end(),
        [&](const InvariantId invId) { return invId != invariantId; }));
    assert(std::all_of(
        invariantQueue.begin(), invariantQueue.end(),
        [&](const InvariantId invId) {
          return inputsToCompute[invariantId] < inputsToCompute[invId] ||
                 (inputsToCompute[invariantId] == inputsToCompute[invId] &&
                  size_t(invariantId) <= size_t(invId));
        }));
    _store.invariant(invariantId).updateBounds(*this, true);

    for (const VarIdBase outputVarId :
         _propGraph.variablesDefinedBy(invariantId)) {
      for (const InvariantId listeningInvariantId :
           listeningInvariants(outputVarId)) {
        // Remove from the data structure must happen before updating
        // inputsToCompute
        if (invariantQueue.contains(listeningInvariantId)) {
          invariantQueue.erase(listeningInvariantId);
        }
        assert(listeningInvariantId != invariantId);
        --inputsToCompute[listeningInvariantId];

        if (inputsToCompute[listeningInvariantId] >= 0) {
          invariantQueue.emplace(listeningInvariantId);
        }
      }
    }
  }
}
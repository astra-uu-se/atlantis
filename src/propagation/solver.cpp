#include "propagation/solver.hpp"

namespace atlantis::propagation {

Solver::Solver()
    : _propagationMode(PropagationMode::INPUT_TO_OUTPUT),
      _propGraph(_store, ESTIMATED_NUM_OBJECTS),
      _outputToInputExplorer(*this, ESTIMATED_NUM_OBJECTS),
      _isEnqueued(ESTIMATED_NUM_OBJECTS),
      _modifiedSearchVariables() {}

PropagationGraph& Solver::propGraph() { return _propGraph; }

void Solver::open() {
  if (_isOpen) {
    throw SolverOpenException("SolverBase already open.");
  }
  if (_solverState != SolverState::IDLE) {
    throw SolverStateException("SolverBase must be idle before opening.");
  }
  _isOpen = true;
}

void Solver::close() {
  if (!_isOpen) {
    throw SolverClosedException("SolverBase already closed.");
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

  // close all invariants
  closeInvariants();

  propagateOnClose();

  assert(_propGraph.propagationQueueEmpty());

  // assert that decsion variable varId is no longer modified.
  assert(std::all_of(_modifiedSearchVariables.begin(),
                     _modifiedSearchVariables.end(), [&](const size_t varId) {
                       return !_store.intVar(varId).hasChanged(
                           _currentTimestamp);
                     }));
}

//---------------------Registration---------------------
void Solver::enqueueComputedVar(VarId id) {
  if (_isEnqueued.get(id)) {
    return;
  }
  _propGraph.enqueuePropagationQueue(id);
  _isEnqueued.set(id, true);
}

void Solver::enqueueComputedVar(VarId id, size_t curLayer) {
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

void Solver::registerInvariantInput(InvariantId invariantId,
                                               VarId inputId, LocalId localId,
                                               bool isDynamicInput) {
  _propGraph.registerInvariantInput(invariantId, sourceId(inputId), localId,
                                    isDynamicInput);
}

void Solver::registerDefinedVariable(VarId varId,
                                                InvariantId invariantId) {
  _propGraph.registerDefinedVariable(sourceId(varId), invariantId);
}

void Solver::registerVar(VarId id) {
  _numVariables++;
  _propGraph.registerVar(id);
  _outputToInputExplorer.registerVar(id);
  _isEnqueued.register_idx(id, false);
}

void Solver::registerInvariant(InvariantId invariantId) {
  _propGraph.registerInvariant(invariantId);
  _outputToInputExplorer.registerInvariant(invariantId);
}

//---------------------Propagation---------------------

VarId Solver::dequeueComputedVar(Timestamp) {
  assert(propagationMode() == PropagationMode::INPUT_TO_OUTPUT ||
         _solverState == SolverState::COMMIT);
  if (_propGraph.propagationQueueEmpty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar(_propGraph.dequeuePropagationQueue());
  _isEnqueued.set(nextVar, false);
  // Due to enqueueComputedVar, all variables in the queue are "active".
  return nextVar;
}

void Solver::clearPropagationQueue() {
  _propGraph.clearPropagationQueue();
  _isEnqueued.assign_all(false);
}

void Solver::closeInvariants() {
  for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
       ++iter) {
    (*iter)->close(_currentTimestamp);
  }
}

void Solver::recomputeAndCommit() {
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
      (*iter)->recompute(_currentTimestamp);
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
    (*iter)->commit(_currentTimestamp);
  }
  clearPropagationQueue();
}

//--------------------- Propagation ---------------------
void Solver::beginMove() {
  assert(!_isOpen);
  assert(_solverState == SolverState::IDLE);

  incCurrentTimestamp();
  _solverState = SolverState::MOVE;
}

void Solver::endMove() {
  assert(_solverState == SolverState::MOVE);
  _solverState = SolverState::IDLE;
}

void Solver::beginProbe() {
  assert(!_isOpen);
  assert(_solverState == SolverState::IDLE);
  _solverState = SolverState::PROBE;
}

void Solver::query(VarId id) {
  assert(!_isOpen);
  assert(_solverState != SolverState::IDLE &&
         _solverState != SolverState::PROCESSING);

  if (_propagationMode != PropagationMode::INPUT_TO_OUTPUT) {
    _outputToInputExplorer.registerForPropagation(_currentTimestamp,
                                                  sourceId(id));
  }
}

void Solver::endProbe() {
  assert(_solverState == SolverState::PROBE);

  _solverState = SolverState::PROCESSING;
  try {
    if (_propagationMode == PropagationMode::INPUT_TO_OUTPUT) {
      if (_propGraph.numLayers() == 1) {
        propagate<CommitMode::NO_COMMIT, true>();
      } else {
        propagate<CommitMode::NO_COMMIT, false>();
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
    _solverState = SolverState::IDLE;
  } catch (std::exception const& e) {
    _solverState = SolverState::IDLE;
    throw e;
  }
}

void Solver::beginCommit() {
  assert(!_isOpen);
  assert(_solverState == SolverState::IDLE);

  _outputToInputExplorer.clearRegisteredVariables();

  _solverState = SolverState::COMMIT;
}

void Solver::endCommit() {
  assert(_solverState == SolverState::COMMIT);

  _solverState = SolverState::PROCESSING;

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
      propagate<CommitMode::COMMIT, true>();
    } else {
      propagate<CommitMode::COMMIT, false>();
    }

    // assert that decsion variable varId is no longer modified.
    assert(_propagationMode != PropagationMode::OUTPUT_TO_INPUT ||
           std::all_of(_modifiedSearchVariables.begin(),
                       _modifiedSearchVariables.end(), [&](const size_t varId) {
                         return !_store.intVar(varId).hasChanged(
                             _currentTimestamp);
                       }));
    _solverState = SolverState::IDLE;
  } catch (std::exception const& e) {
    _solverState = SolverState::IDLE;
    throw e;
  }
}

void Solver::propagateOnClose() {
  IdMap<InvariantId, bool> committedInvariants(_propGraph.numInvariants());
  committedInvariants.assign(_propGraph.numInvariants(), false);
  for (VarId varId : _propGraph.searchVariables()) {
    commitIf(_currentTimestamp, varId);
  }
  for (size_t layer = 0; layer < _propGraph.numLayers(); ++layer) {
    if (_propGraph.hasDynamicCycle(layer)) {
      _propGraph.topologicallyOrder(_currentTimestamp, layer);
    }
    std::vector<VarIdBase> vars(_propGraph.variablesInLayer(layer));
    std::sort(vars.begin(), vars.end(),
              [&](const VarIdBase a, const VarIdBase b) {
                return _propGraph.position(a) < _propGraph.position(b);
              });
    for (const VarIdBase varId : vars) {
      const InvariantId defInv = _propGraph.definingInvariant(varId);
      if (defInv != NULL_ID && !committedInvariants.get(defInv)) {
        committedInvariants.set(defInv, true);
        Invariant& inv = _store.invariant(defInv);
        inv.recompute(_currentTimestamp);
        inv.commit(_currentTimestamp);
      }
      commitIf(_currentTimestamp, varId);
    }
  }
}

template void Solver::propagate<CommitMode::NO_COMMIT, false>();
template void Solver::propagate<CommitMode::NO_COMMIT, true>();
template void Solver::propagate<CommitMode::COMMIT, false>();
template void Solver::propagate<CommitMode::COMMIT, true>();
// Propagates at the current internal timestamp of the solver.
template <CommitMode Mode, bool SingleLayer>
void Solver::propagate() {
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
          defInv.compute(_currentTimestamp);
          for (const VarId inputId : defInv.nonPrimaryDefinedVars()) {
            if (hasChanged(_currentTimestamp, inputId)) {
              assert(!_isEnqueued.get(inputId));
              _propGraph.enqueuePropagationQueue(inputId);
              _isEnqueued.set(inputId, true);
            }
          }
          if constexpr (Mode == CommitMode::COMMIT) {
            defInv.commit(_currentTimestamp);
          }
          if (oldValue == variable.value(_currentTimestamp)) {
            continue;
          }
        }
      }

      if constexpr (Mode == CommitMode::COMMIT) {
        commitIf(_currentTimestamp, queuedVar);
      }

      for (const auto& toNotify : listeningInvariantData(queuedVar)) {
        Invariant& invariant = _store.invariant(toNotify.invariantId);
        assert(invariant.primaryDefinedVar() != NULL_ID);
        assert(toNotify.invariantId != definingInvariant);
        assert(invariant.primaryDefinedVar().idType == VarIdType::var);
        if constexpr (SingleLayer) {
          assert(_propGraph.position(queuedVar) <
                 _propGraph.position(invariant.primaryDefinedVar()));
        } else {
          if (_propGraph.position(queuedVar) >
              _propGraph.position(invariant.primaryDefinedVar())) {
            assert(_propGraph.isDynamicInvariant(toNotify.invariantId) &&
                   _store.dynamicInputVar(_currentTimestamp,
                                          toNotify.invariantId) != queuedVar);
            continue;
          }
        }
        invariant.notify(toNotify.localId);
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

void Solver::computeBounds() {
  IdMap<InvariantId, Int> inputsToCompute(numInvariants());

  for (size_t invariantId = 1u; invariantId <= numInvariants(); ++invariantId) {
    inputsToCompute.register_idx(invariantId,
                                 inputVariables(invariantId).size());
  }

  // Search variables might now have been computed yet
  for (size_t varId = 1u; varId <= numVariables(); ++varId) {
    if (definingInvariant(varId) == NULL_ID) {
      for (const PropagationGraph::ListeningInvariantData&
               listeningInvariantData : listeningInvariantData(varId)) {
        --inputsToCompute[listeningInvariantData.invariantId];
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
    _store.invariant(invariantId).updateBounds(true);

    for (const VarIdBase outputVarId :
         _propGraph.variablesDefinedBy(invariantId)) {
      for (const PropagationGraph::ListeningInvariantData&
               listeningInvariantData : listeningInvariantData(outputVarId)) {
        // Remove from the data structure must happen before updating
        // inputsToCompute
        if (invariantQueue.contains(listeningInvariantData.invariantId)) {
          invariantQueue.erase(listeningInvariantData.invariantId);
        }
        assert(listeningInvariantData.invariantId != invariantId);
        --inputsToCompute[listeningInvariantData.invariantId];

        if (inputsToCompute[listeningInvariantData.invariantId] >= 0) {
          invariantQueue.emplace(listeningInvariantData.invariantId);
        }
      }
    }
  }
}
}  // namespace atlantis::propagation
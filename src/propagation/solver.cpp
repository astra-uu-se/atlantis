#include "atlantis/propagation/solver.hpp"

#include <deque>
#include <iostream>
#include <queue>
#include <set>

namespace atlantis::propagation {

Solver::Solver()
    : _propagationMode(PropagationMode::INPUT_TO_OUTPUT),
      _propGraph(_store, ESTIMATED_NUM_OBJECTS),
      _outputToInputExplorer(*this, ESTIMATED_NUM_OBJECTS),
      _isEnqueued(),
      _modifiedSearchVars() {
  _isEnqueued.reserve(ESTIMATED_NUM_OBJECTS);
}

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
    _layerQueue.resize(_propGraph.numLayers(), std::vector<VarId>{});
    for (size_t layer = 1; layer < _propGraph.numLayers(); ++layer) {
      _layerQueue[layer].resize(_propGraph.numVarsInLayer(layer));
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
  assert(std::all_of(
      searchVars().begin(), searchVars().end(), [&](const VarId varId) {
        return _store.intVar(varId).hasChanged(_currentTimestamp) ==
               _modifiedSearchVars.contains(varId);
      }));

  // close all invariants
  closeInvariants();

  propagateOnClose();

  assert(_propGraph.propagationQueueEmpty());

  // assert that decsion variable varId is no longer modified.
  assert(std::all_of(_modifiedSearchVars.begin(), _modifiedSearchVars.end(),
                     [&](const size_t varId) {
                       return !_store.intVar(varId).hasChanged(
                           _currentTimestamp);
                     }));
}

//---------------------Registration---------------------
void Solver::enqueueDefinedVar(VarId id) {
  assert(id < _isEnqueued.size());
  if (_isEnqueued[id]) {
    return;
  }
  _propGraph.enqueuePropagationQueue(id);
  _isEnqueued[id] = true;
}

void Solver::enqueueDefinedVar(VarId id, size_t curLayer) {
  assert(id < _isEnqueued.size());
  if (_isEnqueued[id]) {
    return;
  }
  const size_t varLayer = _propGraph.varLayer(id);
  if (varLayer == curLayer) {
    _propGraph.enqueuePropagationQueue(id);
  } else {
    assert(
        std::all_of(_layerQueue[varLayer].begin(),
                    _layerQueue[varLayer].begin() + _layerQueueIndex[varLayer],
                    [&](const VarId vId) { return _isEnqueued.at(vId); }));
    _layerQueue[varLayer][_layerQueueIndex[varLayer]] = id;
    ++_layerQueueIndex[varLayer];
  }
  _isEnqueued[id] = true;
}

LocalId Solver::registerInvariantInput(InvariantId invariantId, VarViewId inputId,
                                       bool isDynamicInput) {
  return _propGraph.registerInvariantInput(invariantId, sourceId(inputId).id,
                                           isDynamicInput);
}

void Solver::registerDefinedVar(VarId varId, InvariantId invariantId) {
  _propGraph.registerDefinedVar(varId, invariantId);
}

void Solver::makeAllDynamicInputsInactive(Timestamp ts, InvariantId invId) {
  _propGraph.makeAllDynamicInputsInactive(ts, invId);
}

void Solver::makeDynamicInputInactive(Timestamp ts, InvariantId invId,
                                      LocalId localId) {
  _propGraph.makeDynamicInputInactive(ts, invId, localId);
}

void Solver::makeDynamicInputActive(Timestamp ts, InvariantId invId,
                                    LocalId localId) {
  _propGraph.makeDynamicInputActive(ts, invId, localId);
  assert(_store.dynamicInputVar(ts, invId) ==
         _propGraph.dynamicInputVars(invId).at(localId).varId);
}

void Solver::registerVar(VarId id) {
  _numVars++;
  _propGraph.registerVar(id);
  _outputToInputExplorer.registerVar(id);
  assert(id == _isEnqueued.size());
  _isEnqueued.emplace_back(false);
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
    return NULL_ID;
  }
  return _propGraph.dequeuePropagationQueue();
}

void Solver::clearPropagationQueue() {
  _propGraph.clearPropagationQueue();
  _isEnqueued.assign(_isEnqueued.size(), false);
}

void Solver::closeInvariants() {
  for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
       ++iter) {
    (*iter)->close(_currentTimestamp);
  }
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

void Solver::query(VarViewId id) {
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
             std::all_of(searchVars().begin(), searchVars().end(),
                         [&](const VarId varId) {
                           return _store.intVar(varId).hasChanged(
                                      _currentTimestamp) ==
                                  _modifiedSearchVars.contains(varId);
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

  _outputToInputExplorer.clearRegisteredVars();

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
           std::all_of(searchVars().begin(), searchVars().end(),
                       [&](const VarId varId) {
                         return _store.intVar(varId).hasChanged(
                                    _currentTimestamp) ==
                                _modifiedSearchVars.contains(varId);
                       }));
    if (_propGraph.numLayers() == 1) {
      propagate<CommitMode::COMMIT, true>();
    } else {
      propagate<CommitMode::COMMIT, false>();
    }

    // assert that decsion variable varId is no longer modified.
    assert(_propagationMode != PropagationMode::OUTPUT_TO_INPUT ||
           std::all_of(_modifiedSearchVars.begin(), _modifiedSearchVars.end(),
                       [&](const size_t varId) {
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
  std::vector<bool> committedInvariants(_propGraph.numInvariants());
  committedInvariants.assign(_propGraph.numInvariants(), false);
  for (size_t layer = 0; layer < _propGraph.numLayers(); ++layer) {
    if (_propGraph.hasDynamicCycle(layer)) {
      _propGraph.topologicallyOrder(_currentTimestamp, layer);
    }
    std::vector<VarId> vars(_propGraph.varsInLayer(layer));
    std::sort(vars.begin(), vars.end(), [&](const VarId a, const VarId b) {
      return _propGraph.varPosition(a) < _propGraph.varPosition(b);
    });
    for (const VarId varId : vars) {
      const InvariantId defInv = _propGraph.definingInvariant(varId);
      assert(defInv == NULL_ID || defInv < committedInvariants.size());
      if (defInv != NULL_ID && !committedInvariants[defInv]) {
        committedInvariants[defInv] = true;
        Invariant& inv = _store.invariant(defInv);
        inv.recompute(_currentTimestamp);
        inv.commit(_currentTimestamp);
      }
    }

    for (const VarIdBase& varId : vars) {
      _propGraph.commitOutgoingArcs(_currentTimestamp, varId);
    }
  }

  for (size_t i = 1; i <= _propGraph.numVars(); ++i) {
    commitIf(_currentTimestamp, VarIdBase{i});
    _propGraph.commitOutgoingArcs(_currentTimestamp, VarIdBase{i});
  }
}

template void Solver::enforceInvariant<false>(VarIdBase,
                                              const var::OutgoingArc&, size_t);
template void Solver::enforceInvariant<true>(VarIdBase, const var::OutgoingArc&,
                                             size_t);
// Enforces the invariant of outgoingArc
template <bool SingleLayer>
void Solver::enforceInvariant(VarIdBase queuedVar,
                              const var::OutgoingArc& outgoingArc,
                              size_t curLayer) {
  Invariant& invariant = _store.invariant(outgoingArc.invariantId());
  assert(invariant.primaryDefinedVar() != NULL_ID);
  assert(invariant.primaryDefinedVar().idType == VarIdType::var);
  if constexpr (SingleLayer) {
    assert(_propGraph.position(queuedVar) <
           _propGraph.position(invariant.primaryDefinedVar()));
  } else {
    if (_propGraph.position(queuedVar) >
        _propGraph.position(invariant.primaryDefinedVar())) {
      assert(_propGraph.isDynamicInvariant(outgoingArc.invariantId()) &&
             _store.dynamicInputVar(_currentTimestamp,
                                    outgoingArc.invariantId()) != queuedVar);
      return;
    }
  }
  invariant.notifyInputChanged(_currentTimestamp, outgoingArc.localId());
  if constexpr (SingleLayer) {
    assert(_propGraph.layer(invariant.primaryDefinedVar()) == 0);
    enqueueComputedVar(invariant.primaryDefinedVar());
  } else {
    enqueueComputedVar(invariant.primaryDefinedVar(), curLayer);
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
         queuedVar != NULL_ID;
         queuedVar = dequeueComputedVar(_currentTimestamp)) {
      assert(_propGraph.varLayer(queuedVar) == curLayer);
      // queuedVar has been computed under _currentTimestamp
      propagateDefiningInvariant<Mode>(queuedVar);

      if (!hasChanged(_currentTimestamp, queuedVar)) {
        if constexpr (Mode == CommitMode::COMMIT) {
          _propGraph.commitOutgoingArcs(_currentTimestamp, queuedVar);
        }
      }
      if (!hasChanged(_currentTimestamp, queuedVar)) {
        if constexpr (Mode == CommitMode::COMMIT) {
          _propGraph.commitOutgoingArcs(_currentTimestamp, queuedVar);
        }
        continue;
      }

      auto& arcs = outgoingArcs(queuedVar);

      for (const auto& outgoingArc : arcs.outgoingStatic()) {
        enforceInvariant<SingleLayer>(queuedVar, outgoingArc, curLayer);
      }
      const size_t numActiveArcs =
          arcs.outgoingDynamic().numActive(_currentTimestamp);
      for (size_t i = 0; i < numActiveArcs; ++i) {
        const size_t index =
            arcs.outgoingDynamic().indices()[i].value(_currentTimestamp);

        enforceInvariant<SingleLayer>(
            queuedVar, arcs.outgoingDynamic().arcs()[index], curLayer);
      }

      if constexpr (Mode == CommitMode::COMMIT) {
        commitIf(_currentTimestamp, queuedVar);
        _propGraph.commitOutgoingArcs(_currentTimestamp, queuedVar);
      }
    }
    // Done with propagating current layer.
    if constexpr (SingleLayer) {
      return;
    } else {
      assert(_layerQueueIndex.size() == _propGraph.numLayers());

      // Find next layer that has queued variables:
      do {
        ++curLayer;
      } while (curLayer < _propGraph.numLayers() &&
               _layerQueueIndex[curLayer] == 0);

      if (curLayer >= _propGraph.numLayers()) {
        // All layers have been propogated
        assert(std::all_of(_layerQueueIndex.begin(), _layerQueueIndex.end(),
                           [&](const size_t lqi) { return lqi == 0; }));
        return;
      }
      // There are variables to enqueue for the new layer:
      assert(_layerQueueIndex[curLayer] > 0);
      // Topologically order the new layer if necessary:
      if (_propGraph.hasDynamicCycle(curLayer)) {
        _propGraph.topologicallyOrder(_currentTimestamp, curLayer);
      }
      // Add all queued variables to the propagation queue:
      for (size_t i = 0; i < _layerQueueIndex[curLayer]; ++i) {
        assert(_isEnqueued.at(_layerQueue[curLayer][i]));
        _propGraph.enqueuePropagationQueue(_layerQueue[curLayer][i]);
      }
      _layerQueueIndex[curLayer] = 0;
    }
  }
}

void Solver::computeBounds() {
  std::vector<Int> inputsToCompute;
  inputsToCompute.resize(numInvariants(), 0);

  for (InvariantId invariantId = 0; invariantId < numInvariants();
       ++invariantId) {
    inputsToCompute[invariantId] =
        static_cast<Int>(inputVars(invariantId).size());
  }

  // Search variables might now have been computed yet
  for (VarId varId = 0; varId < numVars(); ++varId) {
    if (definingInvariant(varId) == NULL_ID) {
      auto& listeningInvData = outgoingArcs(varId);
      for (const auto& invariantData : listeningInvData.outgoingStatic()) {
        --inputsToCompute[invariantData.invariantId()];
      }
      for (const auto& invariantData :
           listeningInvData.outgoingDynamic().arcs()) {
        --inputsToCompute[invariantData.invariantId()];
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

  for (InvariantId invariantId = 0; invariantId < numInvariants();
       ++invariantId) {
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

    for (const VarId outputVarId : _propGraph.varsDefinedBy(invariantId)) {
      auto& listeningInvData = outgoingArcs(outputVarId);
      for (const auto& listeningInvData : listeningInvData.outgoingStatic()) {
        // Remove from the data structure must happen before updating
        // inputsToCompute
        if (invariantQueue.contains(listeningInvData.invariantId())) {
          invariantQueue.erase(listeningInvData.invariantId());
        }
        assert(listeningInvData.invariantId() != invariantId);
        --inputsToCompute[listeningInvData.invariantId()];

        if (inputsToCompute[listeningInvData.invariantId()] >= 0) {
          invariantQueue.emplace(listeningInvData.invariantId());
        }
      }
    }
  }
}
}  // namespace atlantis::propagation

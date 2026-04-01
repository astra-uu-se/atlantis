#include "atlantis/propagation/solver.hpp"

#include <algorithm>
#include <deque>
#include <queue>
#include <set>

#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/variables/intVar.hpp"

namespace atlantis::propagation {

Solver::Solver()
    : _propagationMode(PropagationMode::INPUT_TO_OUTPUT),
      _propGraph(_store),
      _outputToInputExplorer(*this) {}

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
  _propGraph.close(currentTimestamp());

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
  assert(std::ranges::all_of(
      searchVars().begin(), searchVars().end(), [&](const VarId varId) {
        return _store.intVar(varId).hasChanged(_currentTimestamp) ==
               _modifiedSearchVars.contains(varId);
      }));

  // close all invariants
  closeInvariants();

  propagateOnClose();

  assert(_propGraph.propagationQueueEmpty());

  // assert that decision variable varId is no longer modified.
  assert(std::ranges::all_of(
      _modifiedSearchVars.begin(), _modifiedSearchVars.end(),
      [&](const size_t varId) {
        return !_store.intVar(varId).hasChanged(_currentTimestamp);
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

void Solver::enqueueDefinedVar(VarId id, size_t layer) {
  assert(id < _isEnqueued.size());
  if (_isEnqueued[id]) {
    return;
  }
  const size_t varLayer = _propGraph.varLayer(id);
  if (varLayer == layer) {
    _propGraph.enqueuePropagationQueue(id);
  } else {
    assert(std::ranges::all_of(
        _layerQueue[varLayer].begin(),
        _layerQueue[varLayer].begin() + _layerQueueIndex[varLayer],
        [&](const VarId vId) { return _isEnqueued.at(vId); }));
    _layerQueue[varLayer][_layerQueueIndex[varLayer]] = id;
    ++_layerQueueIndex[varLayer];
  }
  _isEnqueued[id] = true;
}

void Solver::registerInvariantInput(InvariantId invariantId, VarViewId inputId,
                                    LocalId localId, bool isDynamicInput) {
  _propGraph.registerInvariantInput(invariantId, sourceId(inputId), localId,
                                    isDynamicInput);
}

void Solver::registerDefinedVar(VarId definedVarId, InvariantId invariantId) {
  _propGraph.registerDefinedVar(definedVarId, invariantId);
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
             std::ranges::all_of(searchVars().begin(), searchVars().end(),
                                 [&](const VarId varId) {
                                   return _store.intVar(varId).hasChanged(
                                              _currentTimestamp) ==
                                          _modifiedSearchVars.contains(varId);
                                 }));
      outputToInputPropagate();
    }
    _solverState = SolverState::IDLE;
  } catch (std::exception const&) {
    _solverState = SolverState::IDLE;
    throw;
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
           std::ranges::all_of(searchVars().begin(), searchVars().end(),
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

    // assert that decision variable varId is no longer modified.
    assert(_propagationMode != PropagationMode::OUTPUT_TO_INPUT ||
           std::ranges::all_of(
               _modifiedSearchVars.begin(), _modifiedSearchVars.end(),
               [&](const size_t varId) {
                 return !_store.intVar(varId).hasChanged(_currentTimestamp);
               }));
    _solverState = SolverState::IDLE;
  } catch (std::exception const&) {
    _solverState = SolverState::IDLE;
    throw;
  }
}

void Solver::propagateOnClose() {
  std::vector<bool> committedInvariants(_propGraph.numInvariants());
  committedInvariants.assign(_propGraph.numInvariants(), false);
  for (const VarId varId : _propGraph.searchVars()) {
    commitIf(_currentTimestamp, varId);
  }
  for (size_t layer = 0; layer < _propGraph.numLayers(); ++layer) {
    if (_propGraph.hasDynamicCycle(layer)) {
      _propGraph.topologicallyOrder(_currentTimestamp, layer);
    }
    std::vector<VarId> vars(_propGraph.varsInLayer(layer));
    std::ranges::sort(vars, [&](const VarId a, const VarId b) {
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
         queuedVar != NULL_ID;
         queuedVar = dequeueComputedVar(_currentTimestamp)) {
      assert(_propGraph.varLayer(queuedVar) == curLayer);
      // queuedVar has been computed under _currentTimestamp
      const InvariantId definingInvariant =
          _propGraph.definingInvariant(VarId(queuedVar));

      if (definingInvariant != NULL_ID) {
        // If the variable is a defined var
        Invariant& defInv = _store.invariant(definingInvariant);
        // The usage of primary defined var ensures the following if statement
        // is entered only once per invariant:
        if (queuedVar == defInv.primaryDefinedVar()) {
          // enqueue all modified defined vars:
          for (const VarId defVarId : defInv.nonPrimaryDefinedVars()) {
            if (hasChanged(_currentTimestamp, defVarId)) {
              assert(!_isEnqueued.at(defVarId));
              _propGraph.enqueuePropagationQueue(defVarId);
              _isEnqueued[defVarId] = true;
            }
          }
          if constexpr (Mode == CommitMode::COMMIT) {
            // Commit
            defInv.commit(_currentTimestamp);
          }
        }
      }

      if (!hasChanged(_currentTimestamp, queuedVar)) {
        continue;
      }

      // For each invariant queuedVar is an input to:
      for (const auto& toNotify : listeningInvariantData(queuedVar)) {
        Invariant& invariant = _store.invariant(toNotify.invariantId);
        const VarId primaryDefinedVar = invariant.primaryDefinedVar();
        assert(primaryDefinedVar != NULL_ID);
        assert(toNotify.invariantId != definingInvariant);
        invariant.notifyInputChanged(_currentTimestamp, toNotify.localId);
        if constexpr (SingleLayer) {
          assert(_propGraph.varPosition(queuedVar) <
                 _propGraph.varPosition(primaryDefinedVar));
        } else {
          if (_propGraph.varPosition(queuedVar) >
              _propGraph.varPosition(primaryDefinedVar)) {
            assert(_propGraph.isDynamicInvariant(toNotify.invariantId) &&
                   _store.dynamicInputVar(_currentTimestamp,
                                          toNotify.invariantId) != queuedVar);
            continue;
          }
        }
        if constexpr (SingleLayer) {
          assert(_propGraph.varLayer(primaryDefinedVar) == 0);
          enqueueDefinedVar(primaryDefinedVar);
        } else {
          enqueueDefinedVar(primaryDefinedVar, curLayer);
        }
      }

      if constexpr (Mode == CommitMode::COMMIT) {
        commitIf(_currentTimestamp, queuedVar);
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
        // All layers have been propagated
        assert(std::ranges::all_of(_layerQueueIndex.begin(),
                                   _layerQueueIndex.end(),
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
    assert(std::ranges::all_of(
        invariantQueue.begin(), invariantQueue.end(),
        [&](const InvariantId invId) { return invId != invariantId; }));
    assert(std::ranges::all_of(
        invariantQueue.begin(), invariantQueue.end(),
        [&](const InvariantId invId) {
          return inputsToCompute[invariantId] < inputsToCompute[invId] ||
                 (inputsToCompute[invariantId] == inputsToCompute[invId] &&
                  size_t(invariantId) <= size_t(invId));
        }));
    _store.invariant(invariantId).updateBounds(true);

    for (const VarId outputVarId : _propGraph.varsDefinedBy(invariantId)) {
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

void Solver::incCurrentTimestamp() {
  ++_currentTimestamp;
  if (_propagationMode == PropagationMode::INPUT_TO_OUTPUT) {
    clearPropagationQueue();
  } else {
    _modifiedSearchVars.clear();
  }
  assert(std::ranges::all_of(
      searchVars().begin(), searchVars().end(), [&](const VarId varId) {
        return !_store.intVar(varId).hasChanged(_currentTimestamp);
      }));
}

size_t Solver::numVars() const { return _propGraph.numVars(); }

size_t Solver::numInvariants() const { return _propGraph.numInvariants(); }

InvariantId Solver::definingInvariant(VarViewId id) const {
  return _propGraph.definingInvariant(id.isView() ? sourceId(id) : VarId(id));
}

const std::vector<VarId>& Solver::varsDefinedBy(InvariantId invariantId) const {
  return _propGraph.varsDefinedBy(invariantId);
}

const std::vector<PropagationGraph::ListeningInvariantData>&
Solver::listeningInvariantData(VarId id) const {
  return _propGraph.listeningInvariantData(id);
}

VarId Solver::nextInput(InvariantId invariantId) {
  return sourceId(_store.invariant(invariantId).nextInput(_currentTimestamp));
}
void Solver::notifyCurrentInputChanged(InvariantId invariantId) {
  _store.invariant(invariantId).notifyCurrentInputChanged(_currentTimestamp);
}

void Solver::setValue(Timestamp ts, VarViewId id, Int val) {
  assert(id.isVar());
  setValue(ts, VarId(id), val);
}

void Solver::setValue(Timestamp ts, VarId id, Int val) {
  assert(_propGraph.isSearchVar(id));

  IntVar& var = _store.intVar(id);
  var.setValue(ts, val);

  if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
    if (ts != _currentTimestamp) {
      _modifiedSearchVars.clear();
    }

    if (var.hasChanged(ts)) {
      _modifiedSearchVars.emplace(id);
    } else {
      _modifiedSearchVars.erase(id);
    }
  }
  enqueueDefinedVar(id);
}

void Solver::setPropagationMode(PropagationMode propMode) {
  if (!_isOpen) {
    throw SolverClosedException(
        "Cannot set propagation mode when model is closed");
  }
  _propagationMode = propMode;
}

OutputToInputMarkingMode Solver::outputToInputMarkingMode() const {
  return _outputToInputExplorer.outputToInputMarkingMode();
}

void Solver::setOutputToInputMarkingMode(OutputToInputMarkingMode markingMode) {
  if (!_isOpen) {
    throw SolverClosedException(
        "Cannot set output-to-input marking mode when model is closed");
  }
  _outputToInputExplorer.setOutputToInputMarkingMode(markingMode);
}

const std::vector<VarId>& Solver::searchVars() const {
  return _propGraph.searchVars();
}

const std::vector<std::pair<VarId, bool>>& Solver::inputVars(
    InvariantId invariantId) const {
  return _propGraph.inputVars(invariantId);
}

const std::unordered_set<VarId>& Solver::modifiedSearchVar() const {
  return _modifiedSearchVars;
}

void Solver::outputToInputPropagate() {
  assert(propagationMode() == PropagationMode::OUTPUT_TO_INPUT);
  _outputToInputExplorer.propagate(_currentTimestamp);
}

}  // namespace atlantis::propagation

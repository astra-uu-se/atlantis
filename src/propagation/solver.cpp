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
void Solver::enqueueDefinedVar(const VarId id) {
  assert(id < _isEnqueued.size());
  if (_isEnqueued[id]) {
    return;
  }
  _propGraph.enqueuePropagationQueue(id);
  _isEnqueued[id] = true;
}

void Solver::enqueueDefinedVar(const VarId id, const size_t layer) {
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

LocalId Solver::registerInvariantInput(InvariantId invariantId,
                                       VarViewId inputId, bool isDynamicInput) {
  return _propGraph.registerInvariantInput(invariantId, sourceId(inputId),
                                           isDynamicInput);
}

void Solver::registerDefinedVar(const VarId definedVarId,
                                const InvariantId invariantId) {
  _propGraph.registerDefinedVar(definedVarId, invariantId);
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

void Solver::registerVar(const VarId id) {
  _numVars++;
  _propGraph.registerVar(id);
  _outputToInputExplorer.registerVar(id);
  assert(id == _isEnqueued.size());
  _isEnqueued.emplace_back(false);
  _commitOutgoingEdges.emplace_back(false);
}

void Solver::registerInvariant(const InvariantId invariantId) {
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
  _commitOutgoingEdges.assign(_commitOutgoingEdges.size(), false);
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
  assert(_solverState != SolverState::COMMIT);
  assert(_solverState != SolverState::IDLE);
  assert(_solverState != SolverState::PROBE);
  assert(_solverState != SolverState::PROCESSING);
  assert(_solverState == SolverState::MOVE);
  _solverState = SolverState::IDLE;
}

void Solver::beginProbe() {
  assert(!_isOpen);
  assert(_solverState == SolverState::IDLE);
  _solverState = SolverState::PROBE;
}

void Solver::query(const VarViewId id) {
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
    }
  }

  for (VarId i = 0; i < _propGraph.numVars(); ++i) {
    commitIf(_currentTimestamp, i);
    _propGraph.commitOutgoingEdges(_currentTimestamp, i);
  }
}

template void Solver::enforceInvariant<false>(VarId, const var::OutgoingEdge&,
                                              size_t);
template void Solver::enforceInvariant<true>(VarId, const var::OutgoingEdge&,
                                             size_t);
// Enforces the invariant of outgoingEdges
template <bool SingleLayer>
void Solver::enforceInvariant(VarId queuedVar,
                              const var::OutgoingEdge& outgoingEdge,
                              size_t curLayer) {
  assert(outgoingEdge.invariantId() != NULL_ID);
  Invariant& inv = _store.invariant(outgoingEdge.invariantId());
  assert(inv.primaryDefinedVar() != NULL_ID);
  assert(curLayer < _propGraph.varLayer(inv.primaryDefinedVar()) ||
         _propGraph.varPosition(queuedVar) <
             _propGraph.varPosition(inv.primaryDefinedVar()));
  inv.notifyInputChanged(_currentTimestamp, outgoingEdge.localId());
  if constexpr (SingleLayer) {
    assert(_propGraph.varLayer(inv.primaryDefinedVar()) == 0);
    enqueueDefinedVar(inv.primaryDefinedVar());
  } else {
    enqueueDefinedVar(inv.primaryDefinedVar(), curLayer);
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
      const InvariantId defInvId = _propGraph.definingInvariant(queuedVar);

      if (defInvId != NULL_ID) {
        // If the variable is a defined var
        Invariant& defInv = _store.invariant(defInvId);
        // The usage of primary defined var ensures the following if statement
        // is entered only once per invariant:
        if (queuedVar == defInv.primaryDefinedVar()) {
          // enqueue all modified defined vars:
          for (const VarId outputId : defInv.nonPrimaryDefinedVars()) {
            if (hasChanged(_currentTimestamp, outputId)) {
              assert(!_isEnqueued.at(outputId));
              _propGraph.enqueuePropagationQueue(outputId);
              _isEnqueued[outputId] = true;
            }
          }
          if constexpr (Mode == CommitMode::COMMIT) {
            if (_propGraph.isDynamicInvariant(defInvId)) {
              // the outgoing edge of the dynamic input variables should be
              // committed:
              _commitOutgoingEdges[_store.dynamicInputVar(_currentTimestamp,
                                                          defInvId)] = true;
              _commitOutgoingEdges[_store.dynamicInputVar(_currentTimestamp + 1,
                                                          defInvId)] = true;
            }
            // Commit the invariant:
            defInv.commit(_currentTimestamp);
          }
        }
      }
      if (!hasChanged(_currentTimestamp, queuedVar)) {
        if constexpr (Mode == CommitMode::COMMIT) {
          _commitOutgoingEdges[queuedVar] = true;
        }
        continue;
      }

      auto& edges = outgoingEdges(queuedVar);

      for (const auto& outgoingEdge : edges.staticEdges()) {
        enforceInvariant<SingleLayer>(queuedVar, outgoingEdge, curLayer);
      }
      const size_t numActiveEdges =
          edges.dynamicEdges().numActive(_currentTimestamp);
      for (size_t i = 0; i < numActiveEdges; ++i) {
        assert(i < edges.dynamicEdges().size());
        const auto& edge = edges.dynamicEdge(i, _currentTimestamp);
        assert(edge.invariantId() != NULL_ID);
        const VarId dynVar =
            _store.dynamicInputVar(_currentTimestamp, edge.invariantId());
        assert(queuedVar == dynVar);
        enforceInvariant<SingleLayer>(queuedVar, edge, curLayer);
      }

      if constexpr (Mode == CommitMode::COMMIT) {
        commitIf(_currentTimestamp, queuedVar);
        _commitOutgoingEdges[queuedVar] = true;
      }
    }
    // Done with propagating current layer.
    if constexpr (SingleLayer) {
      break;
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
        break;
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
  if constexpr (Mode == CommitMode::COMMIT) {
    for (VarId varId = 0; varId < numVars(); ++varId) {
      _propGraph.commitOutgoingEdges(currentTimestamp(), varId);
    }
  }
}

void Solver::computeBounds() {
  std::vector<Int> inputsToCompute(numInvariants(), 0);

  for (InvariantId invariantId = 0; invariantId < numInvariants();
       ++invariantId) {
    inputsToCompute[invariantId] = static_cast<Int>(numInputVars(invariantId));
  }

  // Mark the search variables as computed:
  for (VarId varId = 0; varId < numVars(); ++varId) {
    if (definingInvariant(varId) == NULL_ID) {
      auto& listeningInvData = outgoingEdges(varId);
      for (const auto& invariantData : listeningInvData.staticEdges()) {
        --inputsToCompute[invariantData.invariantId()];
      }
      for (const auto& invariantData :
           listeningInvData.dynamicEdges().edges()) {
        --inputsToCompute[invariantData.invariantId()];
      }
    }
  }

  auto cmp = [&](const InvariantId a, const InvariantId b) {
    if (inputsToCompute[a] == inputsToCompute[b]) {
      return a < b;
    }
    return inputsToCompute[a] < inputsToCompute[b];
  };

  // As a hack, we use an ordered set as a priority queue:
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
                  size_t{invariantId} <= size_t{invId});
        }));
    _store.invariant(invariantId).updateBounds(true);

    for (const VarId outputVarId : _propGraph.varsDefinedBy(invariantId)) {
      // Mark each output variable as comptued:
      auto& listeningInvData = outgoingEdges(outputVarId);
      for (const auto& outEdge : listeningInvData.staticEdges()) {
        // Remove from the data structure must happen before updating
        // inputsToCompute
        if (invariantQueue.contains(outEdge.invariantId())) {
          invariantQueue.erase(outEdge.invariantId());
        }
        assert(outEdge.invariantId() != invariantId);
        --inputsToCompute[outEdge.invariantId()];

        if (inputsToCompute[outEdge.invariantId()] >= 0) {
          invariantQueue.emplace(outEdge.invariantId());
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
  assert(std::ranges::all_of(searchVars(), [&](const VarId varId) {
    return !_store.intVar(varId).hasChanged(_currentTimestamp);
  }));
}

size_t Solver::numVars() const { return _propGraph.numVars(); }

InvariantId Solver::definingInvariant(const VarId id) const {
  return _propGraph.definingInvariant(id);
}

void Solver::setValue(const VarId id, const Int val) {
  setValue(_currentTimestamp, id, val);
}

void Solver::setValue(const VarViewId id, const Int val) {
  setValue(_currentTimestamp, id, val);
}

}  // namespace atlantis::propagation

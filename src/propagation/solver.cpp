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

  for (size_t i = 1; i <= _propGraph.numVars(); ++i) {
    commitIf(_currentTimestamp, VarId{i});
    _propGraph.commitOutgoingArcs(_currentTimestamp, VarId{i});
  }

  for (size_t i = 1; i <= _propGraph.numVars(); ++i) {
    commitIf(_currentTimestamp, VarId{i});
    _propGraph.commitOutgoingArcs(_currentTimestamp, VarId{i});
  }
}

template void Solver::enforceInvariant<false>(VarId, const var::OutgoingArc&,
                                              size_t);
template void Solver::enforceInvariant<true>(VarId, const var::OutgoingArc&,
                                             size_t);
// Enforces the invariant of outgoingArc
template <bool SingleLayer>
void Solver::enforceInvariant(VarId queuedVar,
                              const var::OutgoingArc& outgoingArc,
                              size_t curLayer) {
  assert(outgoingArc.invariantId() != NULL_ID);
  Invariant& inv = _store.invariant(outgoingArc.invariantId());
  assert(inv.primaryDefinedVar() != NULL_ID);
  assert(curLayer < _propGraph.varLayer(inv.primaryDefinedVar()) ||
         _propGraph.varPosition(queuedVar) <
             _propGraph.varPosition(inv.primaryDefinedVar()));
  inv.notifyInputChanged(_currentTimestamp, outgoingArc.localId());
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
          for (const VarId inputId : defInv.nonPrimaryDefinedVars()) {
            if (hasChanged(_currentTimestamp, inputId)) {
              assert(!_isEnqueued.at(inputId));
              _propGraph.enqueuePropagationQueue(inputId);
              _isEnqueued[inputId] = true;
            }
          }
          if constexpr (Mode == CommitMode::COMMIT) {
            if (_propGraph.isDynamicInvariant(defInvId)) {
              // commit each dynamic input that was not previously committed
              for (const VarId dynVar : std::array<VarId, 2>{
                       _store.dynamicInputVar(_currentTimestamp, defInvId),
                       _store.dynamicInputVar(_currentTimestamp + 1,
                                              defInvId)}) {
                assert(dynVar == NULL_ID || dynVar < _isEnqueued.size());
                if (dynVar != NULL_ID && !_isEnqueued[dynVar]) {
                  if (_propGraph.varLayer(dynVar) < curLayer ||
                      (_propGraph.varLayer(dynVar) == curLayer &&
                       _propGraph.varPosition(dynVar) <
                           _propGraph.varPosition(queuedVar))) {
                    _isEnqueued[dynVar] = true;
                    _propGraph.commitOutgoingArcs(_currentTimestamp, dynVar);
                  } else {
                    enqueueDefinedVar(dynVar, curLayer);
                  }
                }
              }
            }
            // Commit
            defInv.commit(_currentTimestamp);
          }
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
        assert(i < arcs.outgoingDynamic().size());
        const size_t index =
            arcs.outgoingDynamic().indices()[i].value(_currentTimestamp);
        assert(index < arcs.outgoingDynamic().size());
        const auto& arc = arcs.outgoingDynamic().arcs()[index];
        assert(arc.invariantId() != NULL_ID);
        const VarId dynVar =
            _store.dynamicInputVar(_currentTimestamp, arc.invariantId());
        assert(queuedVar == dynVar);
        enforceInvariant<SingleLayer>(queuedVar, arc, curLayer);
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
    inputsToCompute[invariantId] = static_cast<Int>(numInputVars(invariantId));
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

  auto cmp = [&](const InvariantId a, const InvariantId b) {
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
                  size_t{invariantId} <= size_t{invId});
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

InvariantId Solver::definingInvariant(const VarId id) const {
  return _propGraph.definingInvariant(id);
}

InvariantId Solver::definingInvariant(VarViewId id) const {
  return _propGraph.definingInvariant(id.isView() ? sourceId(id) : VarId{id});
}

const std::vector<VarId>& Solver::varsDefinedBy(
    const InvariantId invariantId) const {
  return _propGraph.varsDefinedBy(invariantId);
}

const std::vector<PropagationGraph::ListeningInvariantData>&
Solver::listeningInvariantData(const VarId id) const {
  return _propGraph.listeningInvariantData(id);
}

VarId Solver::nextInput(const InvariantId invariantId) {
  return sourceId(_store.invariant(invariantId).nextInput(_currentTimestamp));
}
void Solver::notifyCurrentInputChanged(const InvariantId invariantId) {
  _store.invariant(invariantId).notifyCurrentInputChanged(_currentTimestamp);
}

void Solver::setValue(const Timestamp ts, const VarViewId id, const Int val) {
  assert(id.isVar());
  setValue(ts, VarId{id}, val);
}

void Solver::setValue(const VarId id, const Int val) {
  setValue(_currentTimestamp, id, val);
}

void Solver::setValue(const VarViewId id, const Int val) {
  setValue(_currentTimestamp, id, val);
}

void Solver::setValue(const Timestamp ts, const VarId id, const Int val) {
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

void Solver::setPropagationMode(const PropagationMode propMode) {
  if (!_isOpen) {
    throw SolverClosedException(
        "Cannot set propagation mode when model is closed");
  }
  _propagationMode = propMode;
}

OutputToInputMarkingMode Solver::outputToInputMarkingMode() const {
  return _outputToInputExplorer.outputToInputMarkingMode();
}

void Solver::setOutputToInputMarkingMode(
    const OutputToInputMarkingMode markingMode) {
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
    const InvariantId invariantId) const {
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

#include "core/propagationEngine.hpp"

#include <iostream>

PropagationEngine::PropagationEngine()
    : _mode(PropagationMode::INPUT_TO_OUTPUT),
      _numVariables(0),
      _propGraph(ESTIMATED_NUM_OBJECTS),
      _outputToInputExplorer(*this, ESTIMATED_NUM_OBJECTS),
      // _propGraph._propagationQueue(PropagationGraph::PriorityCmp(_propGraph)),
      _isEnqueued(ESTIMATED_NUM_OBJECTS),
      _varIsOnPropagationPath(ESTIMATED_NUM_OBJECTS),
      _propagationPathQueue() {}

PropagationGraph& PropagationEngine::getPropGraph() { return _propGraph; }

void PropagationEngine::open() { _isOpen = true; }

void PropagationEngine::close() {
  ++_currentTime;  // todo: Is it safe to increment time here? What if a user
                   // tried to change a variable but without a begin move?
                   // But we should ignore it anyway then...
  _isOpen = false;
  try {
    _propGraph.close();
  } catch (std::exception e) {
    std::cout << "foo";
  }
  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();
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

void PropagationEngine::registerInvariantDependsOnVar(InvariantId dependent,
                                                      VarId source,
                                                      LocalId localId) {
  auto sourceId = getSourceId(source);
  _propGraph.registerInvariantDependsOnVar(dependent, sourceId);
  _dependentInvariantData[sourceId].emplace_back(
      InvariantDependencyData{dependent, localId});
}

void PropagationEngine::registerDefinedVariable(VarId dependent,
                                                InvariantId source) {
  _propGraph.registerDefinedVariable(getSourceId(dependent), source);
}

void PropagationEngine::registerVar(VarId v) {
  _numVariables++;
  _propGraph.registerVar(v);
  _outputToInputExplorer.registerVar(v);
  _isEnqueued.register_idx(v, false);
  _varIsOnPropagationPath.register_idx(v, false);
}

void PropagationEngine::registerInvariant(InvariantId i) {
  _propGraph.registerInvariant(i);
  _outputToInputExplorer.registerInvariant(i);
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

void PropagationEngine::emptyModifiedVariables() {
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
      (*iter)->recompute(_currentTime, *this);
    }
    for (auto iter = _store.intVarBegin(); iter != _store.intVarEnd(); ++iter) {
      if (iter->hasChanged(_currentTime)) {
        done = false;
        iter->commit();
      }
    }
  }
  // We must commit all invariants once everything is stable.
  // Commiting an invariant will commit any internal datastructure.
  for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
       ++iter) {
    (*iter)->commit(_currentTime, *this);
  }
  emptyModifiedVariables();
}

//--------------------- Move semantics ---------------------
void PropagationEngine::beginMove() {
  assert(!_isMoving);
  _isMoving = true;
  ++_currentTime;
  // only needed for bottom up propagation
  clearPropagationPath();
}

void PropagationEngine::endMove() {
  assert(_isMoving);
  _isMoving = false;
}

void PropagationEngine::beginQuery() {}

void PropagationEngine::query(VarId id) {
  if (_mode == PropagationMode::INPUT_TO_OUTPUT) {
    return;
  }
  _outputToInputExplorer.registerForPropagation(_currentTime, getSourceId(id));
}

void PropagationEngine::endQuery() {
  switch (_mode) {
    case PropagationMode::INPUT_TO_OUTPUT:
      propagate<false>();
      break;
    case PropagationMode::OUTPUT_TO_INPUT:
      if (_useMarkingForOutputToInput) {
        markPropagationPathAndEmptyModifiedVariables();
      } else {
        emptyModifiedVariables();
      }
      outputToInputPropagate<false>();
      break;
    case PropagationMode::MIXED:
      if (_useMarkingForOutputToInput) {
        markPropagationPathAndEmptyModifiedVariables();
      } else {
        emptyModifiedVariables();
      }
      outputToInputPropagate<false>();
      break;
  }
  // We must always clear due to the current version of query()
  _outputToInputExplorer.clearRegisteredVariables();
}

void PropagationEngine::beginCommit() {}

void PropagationEngine::endCommit() {
  switch (_mode) {
    case PropagationMode::INPUT_TO_OUTPUT:
      propagate<true>();
      break;
    case PropagationMode::OUTPUT_TO_INPUT:
      if (_useMarkingForOutputToInput) {
        markPropagationPathAndEmptyModifiedVariables();
      } else {
        emptyModifiedVariables();
      }
      outputToInputPropagate<true>();
      // BUG: Variables that are not dynamically defining the queries variables
      // will not be properly updated: they should be marked as changed until
      // committed but this does not happen.
      // TODO: create test case to catch this bug.
      break;
    case PropagationMode::MIXED:
      propagate<true>();
      break;
  }
  // We must always clear due to the current version of query()
  _outputToInputExplorer.clearRegisteredVariables();

  return;
  // Todo: This just commits everything and can be very inefficient, instead
  // commit during propagation.

  // Commit all variables:
  // for (auto iter = _store.intVarBegin(); iter != _store.intVarEnd();
  // ++iter) {
  //   iter->commitIf(_currentTime);
  // }
  // // Commit all invariants:
  // for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
  //      ++iter) {
  //   (*iter)->commit(_currentTime, *this);
  // }
}

void PropagationEngine::markPropagationPathAndEmptyModifiedVariables() {
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
    for (auto& depInv : _dependentInvariantData.at(currentVar)) {
      for (VarIdBase depVar : _propGraph.getVariablesDefinedBy(depInv.id)) {
        if (!_varIsOnPropagationPath.get(depVar)) {
          _propagationPathQueue.push(depVar);
        }
      }
    }
  }
}

template void PropagationEngine::propagate<true>();
template void PropagationEngine::propagate<false>();

// Propagates at the current internal time of the engine.
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

  for (VarId stableVarId = getNextStableVariable(_currentTime);
       stableVarId.id != NULL_ID;
       stableVarId = getNextStableVariable(_currentTime)) {
    IntVar& variable = _store.getIntVar(stableVarId);

    InvariantId definingInvariant =
        _propGraph.getDefiningInvariant(stableVarId);

#ifdef PROPAGATION_DEBUG
    logDebug("\tPropagating " << variable);
    logDebug("\t\tDepends on invariant: " << definingInvariant);
#endif

    if (definingInvariant != NULL_ID) {
      Invariant& defInv = _store.getInvariant(definingInvariant);
      if (stableVarId == defInv.getPrimaryOutput()) {
        Int oldValue = variable.getValue(_currentTime);
        defInv.compute(_currentTime, *this);
        defInv.queueNonPrimaryOutputVarsForPropagation(_currentTime, *this);
        if (oldValue == variable.getValue(_currentTime)) {
#ifdef PROPAGATION_DEBUG
          logDebug("\t\tVariable did not change after compute: ignoring.");
#endif
          continue;
        }
        if constexpr (DoCommit) {
          defInv.commit(_currentTime, *this);
        }
      }
    }

    if constexpr (DoCommit) {
      commitIf(_currentTime, stableVarId);
    }

    for (auto& toNotify : _dependentInvariantData[stableVarId]) {
      Invariant& invariant = _store.getInvariant(toNotify.id);

#ifdef PROPAGATION_DEBUG
      logDebug("\t\tNotifying invariant:" << toNotify.id << " with localId: "
                                          << toNotify.localId);
#endif
#ifdef PROPAGATION_DEBUG_COUNTING
      notificationCount.at(toNotify.id.id - 1)[variable._id.id] =
          notificationCount.at(toNotify.id.id - 1)[variable._id.id] + 1;
#endif

      invariant.notify(toNotify.localId);
      queueForPropagation(_currentTime, invariant.getPrimaryOutput());
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
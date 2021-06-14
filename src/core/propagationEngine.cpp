#include "core/propagationEngine.hpp"

#include <iostream>

PropagationEngine::PropagationEngine()
    : m_propagationMode(PropagationMode::INPUT_TO_OUTPUT),
      m_numVariables(0),
      m_propGraph(ESTIMATED_NUM_OBJECTS),
      m_outputToInputExplorer(*this, ESTIMATED_NUM_OBJECTS),
      // m_propGraph.m_propagationQueue(PropagationGraph::PriorityCmp(m_propGraph)),
      m_isEnqueued(ESTIMATED_NUM_OBJECTS),
      m_varIsOnPropagationPath(ESTIMATED_NUM_OBJECTS),
      m_propagationPathQueue(),
      m_modifiedDecisionVariables(),
      m_decisionVariablesModifiedAt(NULL_TIMESTAMP) {}

PropagationGraph& PropagationEngine::getPropGraph() { return m_propGraph; }

void PropagationEngine::open() {
  if (m_isOpen) {
    throw EngineOpenException("Engine already open.");
  }
  if (m_engineState != EngineState::IDLE) {
    throw EngineStateException("Engine must be idle before opening.");
  }
  m_isOpen = true;
}

void PropagationEngine::close() {
  if (!m_isOpen) {
    throw EngineClosedException("Engine already closed.");
  }

  ++m_currentTime;  // todo: Is it safe to increment time here? What if a user
                    // tried to change a variable but without a begin move?
                    // But we should ignore it anyway then...
  m_isOpen = false;
  try {
    m_propGraph.close();
  } catch (std::exception const& e) {
    std::cout << "foo";
  }
  if (m_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
    m_outputToInputExplorer.populateAncestors();
  }

#ifndef NDEBUG
  // compute initial values for variables and for (internal datastructure of)
  // invariants
  for (VarIdBase varId : getDecisionVariables()) {
    // Assert that if decision variable varId is modified,
    // then it is in the set of modified decision variables
    assert(m_store.getIntVar(varId).hasChanged(m_currentTime) ==
           (m_modifiedDecisionVariables.find(varId) !=
            m_modifiedDecisionVariables.end()));
  }
#endif
  recomputeAndCommit();
#ifndef NDEBUG
  for (size_t varId : m_modifiedDecisionVariables) {
    // assert that decision variable varId is no longer modified.
    assert(!m_store.getIntVar(varId).hasChanged(m_currentTime));
  }
#endif
}

//---------------------Registration---------------------
void PropagationEngine::notifyMaybeChanged(Timestamp, VarId id) {
  // logDebug("\t\t\tMaybe changed: " << m_store.getIntVar(id));
  if (m_isEnqueued.get(id)) {
    // logDebug("\t\t\talready enqueued");
    return;
  }
  // logDebug("\t\t\tpushed on stack");
  m_propGraph.m_propagationQueue.push(id);
  m_isEnqueued.set(id, true);
}

void PropagationEngine::queueForPropagation(Timestamp, VarId id) {
  // logDebug("\t\t\tMaybe changed: " << m_store.getIntVar(id));
  if (m_isEnqueued.get(id)) {
    // logDebug("\t\t\talready enqueued");
    return;
  }
  // logDebug("\t\t\tpushed on stack");
  m_propGraph.m_propagationQueue.push(id);
  m_isEnqueued.set(id, true);
}

void PropagationEngine::registerInvariantDependsOnVar(InvariantId dependent,
                                                      VarId source,
                                                      LocalId localId) {
  auto sourceId = getSourceId(source);
  m_propGraph.registerInvariantDependsOnVar(dependent, sourceId);
  m_dependentInvariantData[sourceId].emplace_back(
      InvariantDependencyData{dependent, localId});
}

void PropagationEngine::registerDefinedVariable(VarId dependent,
                                                InvariantId source) {
  m_propGraph.registerDefinedVariable(getSourceId(dependent), source);
}

void PropagationEngine::registerVar(VarId v) {
  m_numVariables++;
  m_propGraph.registerVar(v);
  m_outputToInputExplorer.registerVar(v);
  m_isEnqueued.register_idx(v, false);
  m_varIsOnPropagationPath.register_idx(v, false);
}

void PropagationEngine::registerInvariant(InvariantId i) {
  m_propGraph.registerInvariant(i);
  m_outputToInputExplorer.registerInvariant(i);
}

//---------------------Propagation---------------------

VarId PropagationEngine::getNextStableVariable(Timestamp) {
  if (m_propGraph.m_propagationQueue.empty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar(m_propGraph.m_propagationQueue.top());
  m_propGraph.m_propagationQueue.pop();
  m_isEnqueued.set(nextVar, false);
  // Due to notifyMaybeChanged, all variables in the queue are "active".
  return nextVar;
}

void PropagationEngine::clearPropagationQueue() {
  while (!m_propGraph.m_propagationQueue.empty()) {
    m_propGraph.m_propagationQueue.pop();
  }
  m_isEnqueued.assign_all(false);
}

void PropagationEngine::recomputeAndCommit() {
  // TODO: This is a very inefficient way of initialising!
  size_t tries = 0;
  bool done = false;
  while (!done) {
    done = true;
    if (tries++ > m_store.getNumVariables()) {
      throw FailedToInitialise();
    }
    for (auto iter = m_store.invariantBegin(); iter != m_store.invariantEnd();
         ++iter) {
      assert((*iter) != nullptr);
      (*iter)->recompute(m_currentTime, *this);
    }
    for (auto iter = m_store.intVarBegin(); iter != m_store.intVarEnd();
         ++iter) {
      if (iter->hasChanged(m_currentTime)) {
        done = false;
        iter->commit();
      }
    }
  }
  // We must commit all invariants once everything is stable.
  // Commiting an invariant will commit any internal datastructure.
  for (auto iter = m_store.invariantBegin(); iter != m_store.invariantEnd();
       ++iter) {
    (*iter)->commit(m_currentTime, *this);
  }
  clearPropagationQueue();
}

//--------------------- Move semantics ---------------------
void PropagationEngine::beginMove() {
  assert(!m_isOpen);
  assert(m_engineState == EngineState::IDLE);

  ++m_currentTime;
  if (m_propagationMode == PropagationMode::MIXED) {
    clearPropagationPath();
  }

  m_engineState = EngineState::MOVE;
}

void PropagationEngine::endMove() {
  assert(m_engineState == EngineState::MOVE);
  m_engineState = EngineState::IDLE;
}

void PropagationEngine::beginQuery() {
  assert(!m_isOpen);
  assert(m_engineState == EngineState::IDLE);
  m_engineState = EngineState::QUERY;
}

void PropagationEngine::query(VarId id) {
  assert(!m_isOpen);
  assert(m_engineState != EngineState::IDLE &&
         m_engineState != EngineState::PROCESSING);

  if (m_propagationMode != PropagationMode::INPUT_TO_OUTPUT) {
    m_outputToInputExplorer.registerForPropagation(m_currentTime,
                                                   getSourceId(id));
  }
}

void PropagationEngine::endQuery() {
  assert(m_engineState == EngineState::QUERY);

  m_engineState = EngineState::PROCESSING;
  try {
    switch (m_propagationMode) {
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
    m_engineState = EngineState::IDLE;
  } catch (std::exception const& e) {
    m_engineState = EngineState::IDLE;
    throw e;
  }
}

void PropagationEngine::beginCommit() {
  assert(!m_isOpen);
  assert(m_engineState == EngineState::IDLE);

  m_outputToInputExplorer.clearRegisteredVariables();

  m_engineState = EngineState::COMMIT;
}

void PropagationEngine::endCommit() {
  assert(m_engineState == EngineState::COMMIT);

  m_engineState = EngineState::PROCESSING;

  try {
#ifndef NDEBUG
    if (m_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
      for (VarIdBase varId : getDecisionVariables()) {
        // Assert that if decision variable varId is modified,
        // then it is in the set of modified decision variables
        assert(m_store.getIntVar(varId).hasChanged(m_currentTime) ==
               (m_modifiedDecisionVariables.find(varId) !=
                m_modifiedDecisionVariables.end()));
      }
    }
#endif

    propagate<true>();
#ifndef NDEBUG
    if (m_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
      for (size_t varId : m_modifiedDecisionVariables) {
        // assert that decision variable varId is no longer modified.
        assert(!m_store.getIntVar(varId).hasChanged(m_currentTime));
      }
    }
#endif
    m_engineState = EngineState::IDLE;
  } catch (std::exception const& e) {
    m_engineState = EngineState::IDLE;
    throw e;
  }
}

void PropagationEngine::markPropagationPathAndClearPropagationQueue() {
  // We cannot iterate over a priority_queue so we cannot copy it.
  // TODO: replace priority_queue of m_propGraph.m_propagationQueue with custom
  // queue.
  while (!m_propGraph.m_propagationQueue.empty()) {
    auto id = m_propGraph.m_propagationQueue.top();
    m_isEnqueued.set(id, false);
    m_propagationPathQueue.push(id);
    m_propGraph.m_propagationQueue.pop();
  }

  while (!m_propagationPathQueue.empty()) {
    VarId currentVar = m_propagationPathQueue.front();
    m_propagationPathQueue.pop();
    if (m_varIsOnPropagationPath.get(currentVar)) {
      continue;
    }
    m_varIsOnPropagationPath.set(currentVar, true);
    for (auto& depInv : m_dependentInvariantData.at(currentVar)) {
      for (VarIdBase depVar : m_propGraph.getVariablesDefinedBy(depInv.id)) {
        if (!m_varIsOnPropagationPath.get(depVar)) {
          m_propagationPathQueue.push(depVar);
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
      m_store.getNumInvariants());
#endif

  for (VarId stableVarId = getNextStableVariable(m_currentTime);
       stableVarId.id != NULL_ID;
       stableVarId = getNextStableVariable(m_currentTime)) {
    IntVar& variable = m_store.getIntVar(stableVarId);

    InvariantId definingInvariant =
        m_propGraph.getDefiningInvariant(stableVarId);

#ifdef PROPAGATION_DEBUG
    logDebug("\tPropagating " << variable);
    logDebug("\t\tDepends on invariant: " << definingInvariant);
#endif

    if (definingInvariant != NULL_ID) {
      Invariant& defInv = m_store.getInvariant(definingInvariant);
      if (stableVarId == defInv.getPrimaryOutput()) {
        Int oldValue = variable.getValue(m_currentTime);
        defInv.compute(m_currentTime, *this);
        defInv.queueNonPrimaryOutputVarsForPropagation(m_currentTime, *this);
        if (oldValue == variable.getValue(m_currentTime)) {
#ifdef PROPAGATION_DEBUG
          logDebug("\t\tVariable did not change after compute: ignoring.");
#endif
          continue;
        }
        if constexpr (DoCommit) {
          defInv.commit(m_currentTime, *this);
        }
      }
    }

    if constexpr (DoCommit) {
      commitIf(m_currentTime, stableVarId);
    }

    for (auto& toNotify : m_dependentInvariantData[stableVarId]) {
      Invariant& invariant = m_store.getInvariant(toNotify.id);

#ifdef PROPAGATION_DEBUG
      logDebug("\t\tNotifying invariant:" << toNotify.id << " with localId: "
                                          << toNotify.localId);
#endif
#ifdef PROPAGATION_DEBUG_COUNTING
      notificationCount.at(toNotify.id.id - 1)[variable.m_id.id] =
          notificationCount.at(toNotify.id.id - 1)[variable.m_id.id] + 1;
#endif

      invariant.notify(toNotify.localId);
      queueForPropagation(m_currentTime, invariant.getPrimaryOutput());
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
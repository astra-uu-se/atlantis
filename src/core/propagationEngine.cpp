#include "core/propagationEngine.hpp"

PropagationEngine::PropagationEngine()
    : mode(PropagationMode::TOP_DOWN),
      m_numVariables(0),
      m_propGraph(ESTIMATED_NUM_OBJECTS),
      m_bottomUpExplorer(*this, ESTIMATED_NUM_OBJECTS),
      m_modifiedVariables(PropagationGraph::PriorityCmp(m_propGraph)),
      m_isEnqueued(ESTIMATED_NUM_OBJECTS),
      m_varIsOnPropagationPath(ESTIMATED_NUM_OBJECTS),
      m_propagationPathQueue() {}

PropagationGraph& PropagationEngine::getPropGraph() { return m_propGraph; }

void PropagationEngine::open() { m_isOpen = true; }

void PropagationEngine::close() {
  ++m_currentTime;  // todo: Is it safe to increment time here? What if a user
                    // tried to change a variable but without a begin move?
                    // But we should ignore it anyway then...
  m_isOpen = false;
  m_propGraph.close();
  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();
}

//---------------------Registration---------------------
void PropagationEngine::notifyMaybeChanged(Timestamp, VarId id) {
  //  std::cout << "\t\t\tMaybe changed: " << m_store.getIntVar(id) << "\n";
  if (m_isEnqueued.get(id)) {
    //    std::cout << "\t\t\talready enqueued\n";
    return;
  }
  //  std::cout << "\t\t\tpushed on stack\n";
  m_modifiedVariables.push(id);
  m_isEnqueued.set(id, true);
}

void PropagationEngine::queueForPropagation(Timestamp, VarId id) {
  //  std::cout << "\t\t\tMaybe changed: " << m_store.getIntVar(id) << "\n";
  if (m_isEnqueued.get(id)) {
    //    std::cout << "\t\t\talready enqueued\n";
    return;
  }
  //  std::cout << "\t\t\tpushed on stack\n";
  m_modifiedVariables.push(id);
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
  m_bottomUpExplorer.registerVar(v);
  m_isEnqueued.register_idx(v, false);
  m_varIsOnPropagationPath.register_idx(v, false);
}

void PropagationEngine::registerInvariant(InvariantId i) {
  m_propGraph.registerInvariant(i);
  m_bottomUpExplorer.registerInvariant(i);
}

//---------------------Propagation---------------------

VarId PropagationEngine::getNextStableVariable(Timestamp) {
  if (m_modifiedVariables.empty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar(m_modifiedVariables.top());
  m_modifiedVariables.pop();
  m_isEnqueued.set(nextVar, false);
  // Due to notifyMaybeChanged, all variables in the queue are "active".
  return nextVar;
}

void PropagationEngine::clearPropagationQueue() {
  while (!m_modifiedVariables.empty()) {
    m_modifiedVariables.pop();
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
  assert(!m_isMoving);
  m_isMoving = true;
  ++m_currentTime;
  // only needed for bottom up propagation
  clearPropagationPath();
}

void PropagationEngine::endMove() {
  assert(m_isMoving);
  m_isMoving = false;
}

void PropagationEngine::beginQuery() {}

void PropagationEngine::query(VarId id) {
  if (mode == PropagationMode::TOP_DOWN) {
    return;
  }
  m_bottomUpExplorer.registerForPropagation(m_currentTime, getSourceId(id));
}

void PropagationEngine::endQuery() {
  switch (mode) {
    case PropagationMode::TOP_DOWN:
      propagate();
      break;
    case PropagationMode::BOTTOM_UP:
      // If marking is removed the restore the clearPropagationQueue in
      // bottomUpPropagate().
      markPropagationPathAndEmptyModifiedVariables(); 
      bottomUpPropagate();
      break;
    case PropagationMode::MIXED:
      markPropagationPathAndEmptyModifiedVariables();
      bottomUpPropagate();
      break;
  }
  // We must always clear due to the current version of query()
  m_bottomUpExplorer.clearRegisteredVariables();
}

void PropagationEngine::beginCommit() {}

void PropagationEngine::endCommit() {
  switch (mode) {
    case PropagationMode::TOP_DOWN:
      propagate();
      break;
    case PropagationMode::BOTTOM_UP:
      markPropagationPathAndEmptyModifiedVariables();
      bottomUpPropagate();
      // BUG: Variables that are not dynamically defining the queries variables
      // will not be properly updated: they should be marked as changed until
      // committed but this does not happen.
      // TODO: create test case to catch this bug.
      break;
    case PropagationMode::MIXED:
      propagate();
      break;
  }
  // We must always clear due to the current version of query()
  m_bottomUpExplorer.clearRegisteredVariables();

  // Todo: This just commits everything and can be very inefficient, instead
  // commit during propagation.

  // Commit all variables:
  for (auto iter = m_store.intVarBegin(); iter != m_store.intVarEnd(); ++iter) {
    iter->commitIf(m_currentTime);
  }
  // Commit all invariants:
  for (auto iter = m_store.invariantBegin(); iter != m_store.invariantEnd();
       ++iter) {
    (*iter)->commit(m_currentTime, *this);
  }
}

void PropagationEngine::markPropagationPathAndEmptyModifiedVariables() {
  // We cannot iterate over a priority_queue so we cannot copy it.
  // TODO: replace priority_queue of m_modifiedVariables with custom queue.
  while (!m_modifiedVariables.empty()) {
    auto id = m_modifiedVariables.top();
    m_isEnqueued.set(id, false);
    m_propagationPathQueue.push(id);
    m_modifiedVariables.pop();
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

// Propagates at the current internal time of the engine.
void PropagationEngine::propagate() {
//#define PROPAGATION_DEBUG
// #define PROPAGATION_DEBUG_COUNTING
#ifdef PROPAGATION_DEBUG
  std::cout << "Starting propagation\n";
#endif
#ifdef PROPAGATION_DEBUG_COUNTING
  std::vector<std::unordered_map<size_t, Int>> notificationCount(
      m_store.getNumInvariants());
#endif

  for (VarId id = getNextStableVariable(m_currentTime); id.id != NULL_ID;
       id = getNextStableVariable(m_currentTime)) {
    IntVar& variable = m_store.getIntVar(id);

    InvariantId definingInvariant = m_propGraph.getDefiningInvariant(id);

#ifdef PROPAGATION_DEBUG
    std::cout << "\tPropagating " << variable << "\n";
    std::cout << "\t\tDepends on invariant: " << definingInvariant << "\n";
#endif

    if (definingInvariant != NULL_ID) {
      Invariant& defInv = m_store.getInvariant(definingInvariant);
      if (id == defInv.getPrimaryOutput()) {
        Int oldValue = variable.getValue(m_currentTime);
        defInv.compute(m_currentTime, *this);
        defInv.queueNonPrimaryOutputVarsForPropagation(m_currentTime, *this);
        if (oldValue == variable.getValue(m_currentTime)) {
#ifdef PROPAGATION_DEBUG
          std::cout << "\t\tVariable did not change after compute: ignoring."
                    << "\n";
#endif

          continue;
        }
      }
    }

    for (auto& toNotify : m_dependentInvariantData[id]) {
      if (toNotify.id == definingInvariant.id) {
#ifdef PROPAGATION_DEBUG
        std::cout << "\t\tIgnoring cyclic notification:" << toNotify.id << "\n";
#endif
        continue;
      }
      Invariant& invariant = m_store.getInvariant(toNotify.id);

#ifdef PROPAGATION_DEBUG
      std::cout << "\t\tNotifying invariant:" << toNotify.id
                << " with localId: " << toNotify.localId << "\n";
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
  std::cout << "Printing notification counts\n";
  for (int i = 0; i < notificationCount.size(); ++i) {
    std::cout << "\tInvariant " << i + 1 << "\n";
    for (auto [k, v] : notificationCount.at(i)) {
      std::cout << "\t\tVarId(" << k << "): " << v << "\n";
    }
  }
#endif
#ifdef PROPAGATION_DEBUG
  std::cout << "Propagation done\n";
#endif
}

void PropagationEngine::bottomUpPropagate() {
  m_bottomUpExplorer.propagate(m_currentTime);
}

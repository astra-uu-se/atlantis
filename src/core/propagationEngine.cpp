#include "core/propagationEngine.hpp"

#include <queue>

PropagationEngine::PropagationEngine()
    : mode(PropagationMode::TOP_DOWN),
      m_propGraph(ESTIMATED_NUM_OBJECTS),
      m_bottomUpExplorer(*this, ESTIMATED_NUM_OBJECTS),
      m_modifiedVariables(PropagationGraph::PriorityCmp(m_propGraph)),
      m_isEnqueued(),
      m_varIsOnPropagationPath(),
      m_propagationPathQueue() {
  m_varIsOnPropagationPath.reserve(ESTIMATED_NUM_OBJECTS);

  m_isEnqueued.push_back(false);  // initialise NULL_ID for indexing
  m_varIsOnPropagationPath.push_back(false);
}

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
  if (m_isEnqueued.at(id)) {
    //    std::cout << "\t\t\talready enqueued\n";
    return;
  }
  //  std::cout << "\t\t\tpushed on stack\n";
  m_modifiedVariables.push(id);
  m_isEnqueued.at(id) = true;
}

void PropagationEngine::registerInvariantDependsOnVar(InvariantId dependent,
                                                      VarId source,
                                                      LocalId localId) {
  m_propGraph.registerInvariantDependsOnVar(dependent, source);
  m_dependentInvariantData.at(source).emplace_back(
      InvariantDependencyData{dependent, localId, NULL_TIMESTAMP});
}

void PropagationEngine::registerDefinedVariable(VarId dependent,
                                                InvariantId source) {
  m_propGraph.registerDefinedVariable(dependent, source);
}

void PropagationEngine::registerVar(VarId v) {
  m_propGraph.registerVar(v);
  m_bottomUpExplorer.registerVar(v);
  m_isEnqueued.push_back(false);
  m_varIsOnPropagationPath.push_back(false);
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
  VarId nextVar = m_modifiedVariables.top();
  m_modifiedVariables.pop();
  m_isEnqueued.at(nextVar) = false;
  // Due to notifyMaybeChanged, all variables in the queue are "active".
  return nextVar;
}

void PropagationEngine::clearPropagationQueue() {
  while (!m_modifiedVariables.empty()) {
    m_modifiedVariables.pop();
  }
  m_isEnqueued.assign(m_isEnqueued.size(), false);
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
  ++m_currentTime;
  {  // only needed for bottom up propagation
    clearPropagationPath();
  }
}

void PropagationEngine::endMove() {}

void PropagationEngine::beginQuery() {}

void PropagationEngine::query(VarId id) {
  if (mode == PropagationMode::TOP_DOWN) {
    return;
  }
  m_bottomUpExplorer.registerForPropagation(m_currentTime, id);
}

void PropagationEngine::endQuery() {
  switch (mode) {
    case PropagationMode::TOP_DOWN:
      propagate();
      break;
    case PropagationMode::BOTTOM_UP:
      markPropagationPath();
      bottomUpPropagate();
      break;
    case PropagationMode::MIXED:
      markPropagationPath();
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
      markPropagationPath();
      bottomUpPropagate();
      break;
    case PropagationMode::MIXED:
      propagate();
      break;
  }
  // We must always clear due to the current version of query()
  m_bottomUpExplorer.clearRegisteredVariables();
}

void PropagationEngine::markPropagationPath() {
  // We cannot iterate over a priority_queue so we cannot copy it.
  // TODO: replace priority_queue of m_modifiedVariables with custom queue.

  for (size_t i = 0; i < m_isEnqueued.size(); i++) {
    if (m_isEnqueued[i]) {
      m_propagationPathQueue.push(i);
    }
  }
  while (!m_propagationPathQueue.empty()) {
    VarId currentVar = m_propagationPathQueue.front();
    m_propagationPathQueue.pop();
    if (m_varIsOnPropagationPath.at(currentVar)) {
      continue;
    }
    m_varIsOnPropagationPath.at(currentVar) = true;
    for (auto& depInv : m_dependentInvariantData.at(currentVar)) {
      for (VarId depVar : m_propGraph.getVariablesDefinedBy(depInv.id)) {
        if (!m_varIsOnPropagationPath.at(depVar)) {
          m_propagationPathQueue.push(depVar);
        }
      }
    }
  }
}

void PropagationEngine::clearPropagationPath() {
  m_varIsOnPropagationPath.assign(m_varIsOnPropagationPath.size(), false);
}

bool PropagationEngine::isOnPropagationPath(VarId id) {
  return m_varIsOnPropagationPath.at(id);
}

// Propagates at the current internal time of the engine.
void PropagationEngine::propagate() {
  // std::cout<< "Starting propagation\n";
  VarId id = getNextStableVariable(m_currentTime);
  while (id.id != NULL_ID) {
    IntVar& variable = m_store.getIntVar(id);
    // std::cout<< "\tPropagating" << variable << "\n";
    if (variable.hasChanged(m_currentTime)) {
      for (auto& toNotify : m_dependentInvariantData.at(id)) {
        // Also, do not notify invariants that are not active.
        if (!isOnPropagationPath(m_currentTime, toNotify.id)) {
          continue;
        }
        // std::cout<< "\t\tNotifying invariant:" << toNotify.id << "\n";
        m_store.getInvariant(toNotify.id)
            .notifyIntChanged(m_currentTime, *this, toNotify.localId,
                              variable.getValue(m_currentTime));
        toNotify.lastNotification = m_currentTime;
      }
    }
    id = getNextStableVariable(m_currentTime);
  }
  //  std::cout<< "Propagation done\n";
}

void PropagationEngine::bottomUpPropagate() {
  m_bottomUpExplorer.propagate(m_currentTime);
  clearPropagationQueue();
}

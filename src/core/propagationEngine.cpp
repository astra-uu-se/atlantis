#include "core/propagationEngine.hpp"

#include <queue>

PropagationEngine::PropagationEngine()
    : m_propGraph(ESTIMATED_NUM_OBJECTS),
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
  assert(id.idType == VarIdType::var);
  //  std::cout << "\t\t\tpushed on stack\n";
  m_modifiedVariables.push(id);
  m_isEnqueued.at(id) = true;
}

void PropagationEngine::registerInvariantDependsOnVar(InvariantId dependent,
                                                      VarId source,
                                                      LocalId localId) {
  VarId varId = source.idType == VarIdType::var
    ? source
    : m_intVarViewSource.at(source);
  VarId viewId = source.idType == VarIdType::var
    ? NULL_ID
    : source;
  
  m_propGraph.registerInvariantDependsOnVar(dependent, varId);
  m_dependentInvariantData.at(varId).emplace_back(
      InvariantDependencyData{dependent, localId, viewId, NULL_TIMESTAMP});
}

void PropagationEngine::registerDefinedVariable(VarId dependent,
                                                InvariantId source) {
  assert(dependent.idType == VarIdType::var);
  m_propGraph.registerDefinedVariable(dependent, source);
}

void PropagationEngine::registerVar(VarId v) {
  VarId varId = v.idType == VarIdType::var
    ? v
    : m_intVarViewSource.at(v);
  m_propGraph.registerVar(varId);
  m_bottomUpExplorer.registerVar(varId);
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
      if (!iter->hasChanged(m_currentTime)) {
        continue;
      }
      done = false;
      iter->commit();
      assert(iter->getTmpTimestamp() == m_currentTime);
      for (VarId viewId : m_dependantIntVarViews.at(iter->getId())) {
        recomputeUsingParent(viewId, (*iter));
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
  m_bottomUpExplorer.registerForPropagation(
    m_currentTime,
    id.idType == VarIdType::var ? id : m_intVarViewSource.at(id)
  );
}

void PropagationEngine::endQuery() {
  {  // Top down
    // propagate();
  }
  {  // Bottom up
     markPropagationPath();
     bottomUpPropagate();
  }
  // We must always clear due to the current version of query()
  m_bottomUpExplorer.clearRegisteredVariables();
}

void PropagationEngine::beginCommit() {}

void PropagationEngine::endCommit() {
  // Todo: perform top down propagation
  {  // Bottom up
     markPropagationPath();
     bottomUpPropagate();
  }
  // propagate();
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
        assert(depVar.idType == VarIdType::var);
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
  
  for (VarId id = getNextStableVariable(m_currentTime);
        id.id != NULL_ID;
        id = getNextStableVariable(m_currentTime)) {
    IntVar& variable = m_store.getIntVar(id);
    // std::cout<< "\tPropagating" << variable << "\n";
    if (!variable.hasChanged(m_currentTime)) {
      continue;
    }
    for (auto viewId : m_dependantIntVarViews.at(id)) {
      recomputeUsingParent(viewId, variable);
    }
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
  //  std::cout<< "Propagation done\n";
}

void PropagationEngine::bottomUpPropagate() {
  m_bottomUpExplorer.propagate(m_currentTime);
  clearPropagationQueue();
}
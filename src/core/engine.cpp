#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

extern Id NULL_ID;

Engine::Engine()
    : m_currentTime(NULL_TIMESTAMP + 1),
      // m_intVars(),
      // m_invariants(),
      m_propGraph(*this, ESTIMATED_NUM_OBJECTS),
      m_isOpen(false),
      m_store(ESTIMATED_NUM_OBJECTS, NULL_ID) {
  m_intVarViewSource.reserve(ESTIMATED_NUM_OBJECTS);
  m_intVarViewSource.push_back(NULL_ID);

  m_dependantIntVarViews.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependantIntVarViews.push_back({});

  m_dependentInvariantData.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependentInvariantData.push_back({});
}

inline void Engine::recomputeUsingParent(VarId viewId, IntVar& var) {
  assert(viewId.idType == VarIdType::view);
  return recomputeUsingParent(m_store.getIntVarView(viewId), var);
}

inline void Engine::recomputeUsingParent(IntVarView& view, IntVar& var) {
  VarId parentId = view.getParentId();
  Timestamp t;
  if (parentId.idType == VarIdType::var) {
    t = var.getTmpTimestamp();
    view.recompute(t, var.getValue(t), var.getCommittedValue());
    return;
  }
  IntVarView& parent = m_store.getIntVarView(parentId);
  t = parent.getTmpTimestamp();
  view.recompute(t, parent.getValue(t), parent.getCommittedValue());
}

void Engine::open() { m_isOpen = true; }

Int Engine::getValue(Timestamp t, VarId v) {
  if (v.idType == VarIdType::var) {
    return m_store.getIntVar(v).getValue(t);
  }
  auto& intVarView = m_store.getIntVarView(v);
  if (intVarView.getTmpTimestamp() == t) {
    return intVarView.getValue(t);
  }
  VarId sourceVarId = m_intVarViewSource.at(v);
  IntVar& sourceVar = m_store.getIntVar(sourceVarId);
  if (sourceVar.getTmpTimestamp() != t) {
    return intVarView.getValue(t);
  }
  if (intVarView.getParentId().idType == VarIdType::var) {
    recomputeUsingParent(intVarView, sourceVar);
    return intVarView.getValue(t);
  }
  auto queue = std::make_unique<std::queue<IntVarView*>>();
  queue->push(&intVarView);
  
  Int prevVal = sourceVar.getValue(t);

  while (queue->back()->getParentId().idType == VarIdType::view) {
    IntVarView& current = m_store.getIntVarView(queue->back()->getParentId());
    // Quick release if current's value is as recent as
    // the source VarId's value
    if (current.getTmpTimestamp() == t) {
      prevVal = current.getValue(t);
      break;
    }
    queue->push(&current);
  }
  
  while (!queue->empty()) {
    queue->front()->recompute(t, prevVal);
    prevVal = queue->front()->getValue(t);
    queue->pop();
  }

  return prevVal;
}

void Engine::recomputeAndCommit() {
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
      // (*iter)->commit(m_currentTime, *this);
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
}

void Engine::close() {
  ++m_currentTime;  // todo: Is it safe to increment time here? What if a user
                    // tried to change a variable but without a begin move?
                    // But we should ignore it anyway then...
  m_isOpen = false;
  m_propGraph.close();
  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();
}

//---------------------Notificaion/Modification---------------------
void Engine::notifyMaybeChanged(Timestamp t, VarId id) {
  if (id.idType == VarIdType::var) {
    m_propGraph.notifyMaybeChanged(t, id);
  } else {
    m_propGraph.notifyMaybeChanged(t, m_intVarViewSource.at(id));
  }
  
}

//--------------------- Move semantics ---------------------
void Engine::beginMove() { ++m_currentTime; }

void Engine::endMove() {}

void Engine::beginQuery() { m_propGraph.clearForPropagation(); }

void Engine::query(VarId id) {
  if (id.idType == VarIdType::var) {
    m_propGraph.registerForPropagation(m_currentTime, id);
  } else {
    m_propGraph.registerForPropagation(m_currentTime, m_intVarViewSource.at(id));
  }
}

void Engine::endQuery() {
  // m_propGraph.schedulePropagation(m_currentTime, *this);
  // propagate();
  // m_propGraph.propagate(m_currentTime);
  m_propGraph.propagate2(m_currentTime);
}

void Engine::beginCommit() { m_propGraph.clearForPropagation(); }

void Engine::endCommit() {
  // m_propGraph.fullPropagateAndCommit(m_currentTime);
  m_propGraph.lazyPropagateAndCommit(m_currentTime);
}

// Propagates at the current internal time of the engine.
void Engine::propagate() {
  for (VarId id = m_propGraph.getNextStableVariable(m_currentTime);
        id != NULL_ID;
        id = m_propGraph.getNextStableVariable(m_currentTime)) {
    IntVar& variable = m_store.getIntVar(id);
    if (!variable.hasChanged(m_currentTime)) {
      continue;
    }
    for (VarId viewId : m_dependantIntVarViews.at(id)) {
      recomputeUsingParent(viewId, variable);
    }
    for (auto& toNotify : m_dependentInvariantData.at(id)) {
      // If we do multiple "probes" within the same timestamp then the
      // invariant may already have been notified.
      // Also, do not notify invariants that are not active.
      if (m_currentTime == toNotify.lastNotification ||
          !m_propGraph.isActive(m_currentTime, toNotify.id)) {
        continue;
      }
      m_store.getInvariant(toNotify.id)
          .notifyIntChanged(m_currentTime, *this, toNotify.localId,
                            variable.getValue(m_currentTime));
      toNotify.lastNotification = m_currentTime;
    }
  }
}

//---------------------Registration---------------------

VarId Engine::makeIntVar(Int initValue, Int lowerBound, Int upperBound) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId =
      m_store.createIntVar(m_currentTime, initValue, lowerBound, upperBound);
  assert(newId.idType == VarIdType::var);
  m_propGraph.registerVar(newId);
  assert(newId.id == m_dependantIntVarViews.size());
  m_dependantIntVarViews.push_back({});
  assert(newId.id == m_dependentInvariantData.size());
  m_dependentInvariantData.push_back({});
  return newId;
}

void Engine::registerInvariantDependsOnVar(InvariantId dependent, VarId source,
                                           LocalId localId) {
  VarId varId = source.idType == VarIdType::view
   ? m_intVarViewSource.at(source)
   : source;

  VarId varViewId = source.idType == VarIdType::view
   ? source
   : NULL_ID;
  
  m_propGraph.registerInvariantDependsOnVar(dependent, varId);
  m_dependentInvariantData.at(varId).emplace_back(
      InvariantDependencyData{dependent, localId, varViewId, NULL_TIMESTAMP});
}

void Engine::registerDefinedVariable(VarId dependent, InvariantId source) {
  assert(dependent.idType == VarIdType::var);
  m_propGraph.registerDefinedVariable(dependent, source);
}

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
  m_dependentInvariantData.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependentInvariantData.push_back({});
}

void Engine::open() { m_isOpen = true; }

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
}

void Engine::close() {
  m_isOpen = false;
  m_propGraph.close();
  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();
}

//---------------------Notificaion/Modification---------------------
void Engine::notifyMaybeChanged(const Timestamp& t, VarId id) {
  m_propGraph.notifyMaybeChanged(t, id);
}

//--------------------- Move semantics ---------------------
void Engine::beginMove() { ++m_currentTime; }

void Engine::endMove() {}


void Engine::commit(VarId& v) {
  m_store.getIntVar(v).commit();
  // todo: do something else? like:
  // m_store.getIntVar(v).validate();
}

void Engine::commitIf(const Timestamp& t, VarId& v) {
  m_store.getIntVar(v).commitIf(t);
  // todo: do something else? like:
  // m_store.getIntVar(v).validate();
}

void Engine::commitValue(VarId& v, Int val) {
  m_store.getIntVar(v).commitValue(val);
  // todo: do something else? like:
  // m_store.getIntVar(v).validate();
}

void Engine::beginQuery() { m_propGraph.clearForPropagation(); }

void Engine::query(VarId id) {
  m_propGraph.registerForPropagation(m_currentTime, id);
}

void Engine::endQuery() {
  // m_propGraph.schedulePropagation(m_currentTime, *this);
  // propagate();
  m_propGraph.propagate(m_currentTime);
}

// Propagates at the current internal time of the engine.
void Engine::propagate() {
  VarId id = m_propGraph.getNextStableVariable(m_currentTime);
  while (id.id != NULL_ID) {
    IntVar& variable = m_store.getIntVar(id);
    if (variable.hasChanged(m_currentTime)) {
      for (auto toNotify : m_dependentInvariantData.at(id)) {
        // If we do multiple "probes" within the same timestamp then the
        // invariant may already have been notified.
        // Also, do not notify invariants that are not active.
        if (m_currentTime == toNotify.lastNotification ||
            !m_propGraph.isActive(m_currentTime, toNotify.id)) {
          continue;
        }
        m_store.getInvariant(toNotify.id)
            .notifyIntChanged(m_currentTime, *this, toNotify.localId,
                              variable.getCommittedValue(),
                              variable.getValue(m_currentTime), toNotify.data);
        toNotify.lastNotification = m_currentTime;
      }
    }
    id = m_propGraph.getNextStableVariable(m_currentTime);
  }
}

//---------------------Registration---------------------

VarId Engine::makeIntVar(Int initValue, Int lowerBound, Int upperBound) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId = m_store.createIntVar(initValue, lowerBound, upperBound);
  m_propGraph.registerVar(newId);
  assert(newId.id == m_dependentInvariantData.size());
  m_dependentInvariantData.push_back({});
  return newId;
}

void Engine::registerInvariantDependsOnVar(InvariantId dependent, VarId source,
                                           LocalId localId, Int data) {
  m_propGraph.registerInvariantDependsOnVar(dependent, source);
  m_dependentInvariantData.at(source).emplace_back(
      InvariantDependencyData{dependent, localId, data, NULL_TIMESTAMP});
}

void Engine::registerDefinedVariable(VarId dependent, InvariantId source) {
  m_propGraph.registerDefinedVariable(dependent, source);
}

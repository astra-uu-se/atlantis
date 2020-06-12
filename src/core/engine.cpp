#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

extern Id NULL_ID;

Engine::Engine()
    : m_currentTime(0),
      // m_intVars(),
      // m_invariants(),
      m_propGraph(ESTIMATED_NUM_OBJECTS),
      m_store(ESTIMATED_NUM_OBJECTS, NULL_ID) {
  m_dependentInvariantData.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependentInvariantData.push_back({});
}

void Engine::open() { m_isOpen = true; }

void Engine::recomputeAndCommit() {
  for (auto iter = m_store.invariantBegin(); iter != m_store.invariantEnd();
       ++iter) {
    assert((*iter) != nullptr);
    (*iter)->recompute(m_currentTime, *this);
    (*iter)->commit(m_currentTime, *this);
  }
  for (auto iter = m_store.intVarBegin(); iter != m_store.intVarEnd(); ++iter) {
    iter->commit();
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
void Engine::notifyMaybeChanged([[maybe_unused]] const Timestamp& t, VarId id) {
  // If first time variable is invalidated:
  if (m_store.getIntVar(id).m_isInvalid) {
    m_store.getIntVar(id).invalidate(t);
    m_propGraph.notifyMaybeChanged(t, id);
  }
}

//--------------------- Move semantics ---------------------
void beginMove([[maybe_unused]] Timestamp& t) {}

void endMove([[maybe_unused]] Timestamp& t) {}

void Engine::setValue(VarId& v, Int val) {
  m_store.getIntVar(v).setValue(m_currentTime, val);
  notifyMaybeChanged(m_currentTime, v);
}

void Engine::setValue(const Timestamp& t, VarId& v, Int val) {
  m_store.getIntVar(v).setValue(t, val);
  notifyMaybeChanged(t, v);
}

void Engine::incValue(const Timestamp& t, VarId& v, Int inc) {
  m_store.getIntVar(v).incValue(t, inc);
  notifyMaybeChanged(t, v);
}

Int Engine::getValue(const Timestamp& t, VarId& v) {
  return m_store.getIntVar(v).getValue(t);
}

Int Engine::getCommitedValue(VarId& v) {
  return m_store.getIntVar(v).getCommittedValue();
}

Timestamp Engine::getTimestamp(VarId& v) {
  return m_store.getIntVar(v).getTimestamp();
}

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

void Engine::commitValue([[maybe_unused]] const Timestamp& t, VarId& v,
                         Int val) {
  m_store.getIntVar(v).commitValue(val);
  // todo: do something else? like:
  // m_store.getIntVar(v).validate();
}

//---------------------Registration---------------------

VarId Engine::makeIntVar() {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId = m_store.createIntVar();
  m_propGraph.registerVar(newId);
  assert(newId.id == m_dependentInvariantData.size());
  m_dependentInvariantData.push_back({});
  return newId;
}

void Engine::registerInvariantDependsOnVar(InvariantId dependent, VarId source,
                                           LocalId localId, Int data) {
  m_propGraph.registerInvariantDependsOnVar(dependent, source);
  m_dependentInvariantData.at(source).emplace_back(
      InvariantDependencyData{dependent, localId, data});
}

void Engine::registerDefinedVariable(VarId dependent, InvariantId source) {
  m_propGraph.registerDefinedVariable(dependent, source);
}
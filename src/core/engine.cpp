#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

extern Id NULL_ID;

Engine::Engine()
    : m_currentTime(0),
      // m_intVars(),
      // m_invariants(),
      m_propGraph(ESTIMATED_NUM_OBJECTS),
      m_store(ESTIMATED_NUM_OBJECTS, NULL_ID) {}


void Engine::open() {
  m_isOpen = true;
}

void Engine::close() {
  m_isOpen = false;
  // TODO: topological sort of the propGraph
  
  // compute and initialise all variables and invariants
  m_store.recomputeAndCommit(m_currentTime, *this);
}

//---------------------Notificaion/Modification---------------------
void Engine::notifyMaybeChanged([[maybe_unused]] const Timestamp& t, VarId id) {
  // If first time variable is invalidated:
  if (m_store.getIntVar(id).m_isInvalid) {
    m_store.getIntVar(id).invalidate(t);
    m_propGraph.notifyMaybeChanged(t, id);
  }
}

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
    throw new ModelNotOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId = m_store.createIntVar();
  m_propGraph.registerVar(newId);
  return newId;
}

void Engine::registerInvariantDependsOnVar(InvariantId dependee, VarId source,
                                           LocalId localId, Int data) {
  m_propGraph.registerInvariantDependsOnVar(dependee, source, localId, data);
}

void Engine::registerDefinedVariable(VarId dependee, InvariantId source) {
  m_propGraph.registerDefinedVariable(dependee, source);
}
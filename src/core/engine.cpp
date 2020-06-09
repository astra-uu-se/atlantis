#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

extern Id NULL_ID;

Engine::Engine()
    : m_currentTime(0),
      m_intVars(),
      m_invariants(),
      m_propGraph(ESTIMATED_NUM_OBJECTS) {
  m_intVars.reserve(ESTIMATED_NUM_OBJECTS);
  m_invariants.reserve(ESTIMATED_NUM_OBJECTS);

  // TODO: since there will be a level of indirection, we do not need to pad
  // m_intVar with null...

  // Vectors indexed by IDs are initialised to size 1 so that the nullID is its
  // only initial member.
  m_intVars.push_back(nullptr);     // expands with registerIntVar
  m_invariants.push_back(nullptr);  // expands with registerInvariant
}

Engine::~Engine() {}

//---------------------Notificaion/Modification---------------------
void Engine::notifyMaybeChanged([[maybe_unused]] const Timestamp& t, Id id) {
  // If first time variable is invalidated:
  if (m_intVars.at(id)->m_isInvalid) {
    m_intVars.at(id)->invalidate(t);
    m_propGraph.notifyMaybeChanged(t, id);
  }
}

void Engine::setValue(const Timestamp& t, VarId& v, Int val) {
  m_intVars.at(v)->setValue(t, val);
  notifyMaybeChanged(t, v);
}

void Engine::incValue(const Timestamp& t, VarId& v, Int inc) {
  m_intVars.at(v)->incValue(t, inc);
  notifyMaybeChanged(t, v);
}

Int Engine::getValue(const Timestamp& t, VarId& v) {
  return m_intVars.at(v)->getValue(t);
}

Int Engine::getCommitedValue(VarId& v) {
  return m_intVars.at(v)->getCommittedValue();
}

Timestamp Engine::getTimestamp(VarId& v) {
  return m_intVars.at(v)->getTimestamp();
}

void Engine::commit(VarId& v) {
  m_intVars.at(v)->commit();
  // todo: do something else? like:
  // m_intVars.at(v)->validate();
}

void Engine::commitIf(const Timestamp& t, VarId& v) {
  m_intVars.at(v)->commitIf(t);
  // todo: do something else? like:
  // m_intVars.at(v)->validate();
}

void Engine::commitValue([[maybe_unused]] const Timestamp& t, VarId& v,
                         Int val) {
  m_intVars.at(v)->commitValue(val);
  // todo: do something else? like:
  // m_intVars.at(v)->validate();
}

//---------------------Registration---------------------

InvariantId Engine::registerInvariant(std::shared_ptr<Invariant> invariantPtr) {
  InvariantId newId = InvariantId(m_invariants.size());
  invariantPtr->setId(newId);

  // TODO : change argument to Invariant
  m_invariants.push_back(invariantPtr);

  m_propGraph.registerInvariant(newId);

  invariantPtr->init(m_currentTime, *this);
  return newId;
}

VarId Engine::makeIntVar() {
  VarId newId = VarId(m_intVars.size());
  m_intVars.emplace_back(std::make_shared<IntVar>(newId));
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
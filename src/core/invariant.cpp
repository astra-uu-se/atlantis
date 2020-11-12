#include "core/invariant.hpp"

#include "core/propagationEngine.hpp"

void Invariant::notify(Timestamp t, Engine& e, LocalId id) {
  m_modifiedVars.push(id);

  if (!m_isModified) {
    e.queueForPropagation(t, m_primaryOutput);
    for (VarId outId : m_outputVars) {
      e.queueForPropagation(t, outId);
    }
  }
  m_isModified = true;
}

void Invariant::compute(Timestamp t, Engine& e) {
  assert(m_modifiedVars.size() > 0);
  assert(m_primaryOutput != NULL_ID);

  while (m_modifiedVars.hasNext()) {
    // don't turn this into a for loop...
    LocalId toNotify = m_modifiedVars.pop();
    this->notifyIntChanged(t, e, toNotify);
  }

  m_isModified = false;
}

void Invariant::registerDefinedVariable(Engine& e, VarId v) {
  if (m_primaryOutput == NULL_ID) {
    m_primaryOutput = v;
  } else {
    m_outputVars.push_back(v);
  }
  e.registerDefinedVariable(v, m_id);
}

void Invariant::updateValue(Timestamp t, Engine& e, VarId id, Int val) {
  e.updateValue(t, id, val);
}

void Invariant::incValue(Timestamp t, Engine& e, VarId id, Int val) {
  e.incValue(t, id, val);
}
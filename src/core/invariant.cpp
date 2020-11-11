#include "core/invariant.hpp"

#include "core/propagationEngine.hpp"

void Invariant::notify(Timestamp t, Engine& e,LocalId id){
  m_modifiedVars[id] = true;
  if(!m_isModified){
    e.queueForPropagation(t,m_primaryOutput);
    for(VarId outId: m_outputVars){
      e.queueForPropagation(t,outId);
    }
  }
  m_isModified = true;
}

void Invariant::compute(Timestamp t, Engine& e) {
  assert(m_modifiedVars.size() > 0);
  assert(m_primaryOutput != NULL_ID);
  for (size_t i = 0; i < m_modifiedVars.size(); ++i) {
    if (m_modifiedVars[i]) {
      this->notifyIntChanged(t, e, LocalId(i));
      m_modifiedVars[i] = false;  // reset
    }
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
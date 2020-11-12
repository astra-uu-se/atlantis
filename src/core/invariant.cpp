#include "core/invariant.hpp"

#include "core/propagationEngine.hpp"

void Invariant::notify(LocalId id) {
  m_modifiedVars.push(id);
}

void Invariant::compute(Timestamp t, Engine& e) {
  assert(m_modifiedVars.size() > 0);
  assert(m_primaryOutput != NULL_ID);

  while (m_modifiedVars.hasNext()) {
    // don't turn this into a for loop...
    LocalId toNotify = m_modifiedVars.pop();
    this->notifyIntChanged(t, e, toNotify);
  }
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

void Invariant::queueNonPrimaryOutputVarsForPropagation(Timestamp t, Engine& e) {
  for (VarId outputVarId : m_outputVars) {
    e.queueForPropagation(t, outputVarId);
  }
}
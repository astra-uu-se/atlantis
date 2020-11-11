#include "core/invariant.hpp"
#include "core/propagationEngine.hpp"

void Invariant::compute(Timestamp t, Engine& e){
  assert(m_modifiedVars.size() > 0);
  assert(m_primaryOutput != NULL_ID);
  for (size_t i = 0; i < m_modifiedVars.size(); ++i) {
    if(m_modifiedVars[i]){
      this->notifyIntChanged(t,e,LocalId(i));
      m_modifiedVars[i] = false; // reset
    }
  }

  e.notifyMaybeChanged(t, m_primaryOutput);
  for(VarId output: m_outputVars){
    e.notifyMaybeChanged(t, output);
  }
}

void Invariant::registerDefinedVariable(Engine& e, VarId v){
  if(m_primaryOutput == NULL_ID){
    m_primaryOutput = v;
  }else {
    m_outputVars.push_back(v);
  }
  e.registerDefinedVariable(v, m_id);
}
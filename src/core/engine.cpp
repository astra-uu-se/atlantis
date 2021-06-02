#include "core/engine.hpp"

#include "variables/intVar.hpp"

Engine::Engine()
    : m_currentTime(NULL_TIMESTAMP + 1),
      m_isOpen(false),
      m_dependentInvariantData(ESTIMATED_NUM_OBJECTS),
      m_store(ESTIMATED_NUM_OBJECTS, NULL_ID) {}

//---------------------Registration---------------------

VarId Engine::makeIntVar(Int initValue, Int lowerBound, Int upperBound) {
  if (!m_isOpen) {
    throw EngineOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId =
      m_store.createIntVar(m_currentTime, initValue, lowerBound, upperBound);
  registerVar(newId);
  m_dependentInvariantData.register_idx(newId);
  return newId;
}
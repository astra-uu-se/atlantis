#include "core/engine.hpp"

#include "exceptions/exceptions.hpp"

extern Id NULL_ID;

Engine::Engine()
    : m_currentTime(NULL_TIMESTAMP + 1),
      m_isOpen(false),
      m_store(ESTIMATED_NUM_OBJECTS, NULL_ID) {
  m_intVarViewSource.reserve(ESTIMATED_NUM_OBJECTS);
  m_intVarViewSource.push_back(NULL_ID);

  m_dependantIntVarViews.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependantIntVarViews.push_back({});

  m_dependentInvariantData.reserve(ESTIMATED_NUM_OBJECTS);
  m_dependentInvariantData.push_back({});
}

//---------------------Registration---------------------

VarId Engine::makeIntVar(Int initValue, Int lowerBound, Int upperBound) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId =
      m_store.createIntVar(m_currentTime, initValue, lowerBound, upperBound);
  registerVar(newId);
  assert(newId.id == m_dependentInvariantData.size());
  m_dependentInvariantData.push_back({});
  return newId;
}
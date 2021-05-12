#include "core/engine.hpp"

#include "variables/intVar.hpp"

Engine::Engine()
    : _currentTime(NULL_TIMESTAMP + 1),
      _isOpen(false),
      _dependentInvariantData(ESTIMATED_NUM_OBJECTS),
      _store(ESTIMATED_NUM_OBJECTS, NULL_ID) {}

//---------------------Registration---------------------

VarId Engine::makeIntVar(Int initValue, Int lowerBound, Int upperBound) {
  if (!_isOpen) {
    throw ModelNotOpenException("Cannot make IntVar when store is closed.");
  }
  VarId newId =
      _store.createIntVar(_currentTime, initValue, lowerBound, upperBound);
  registerVar(newId);
  _dependentInvariantData.register_idx(newId);
  return newId;
}
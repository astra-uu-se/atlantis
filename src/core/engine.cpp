#include "core/engine.hpp"

#include "variables/intVar.hpp"

Engine::Engine()
    : _currentTimestamp(NULL_TIMESTAMP + 1),
      _isOpen(false),
      _store(ESTIMATED_NUM_OBJECTS, NULL_ID) {}

//---------------------Registration---------------------

VarId Engine::makeIntVar(Int initValue, Int lowerBound, Int upperBound) {
  if (!_isOpen) {
    throw EngineClosedException("Cannot make IntVar when store is closed.");
  }
  VarId newId =
      _store.createIntVar(_currentTimestamp, initValue, lowerBound, upperBound);
  registerVar(newId);
  return newId;
}
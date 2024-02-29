#include "atlantis/propagation/solver.hpp"

namespace atlantis::propagation {

SolverBase::SolverBase()
    : _currentTimestamp(NULL_TIMESTAMP + 1),
      _isOpen(false),
      _store(ESTIMATED_NUM_OBJECTS, NULL_ID) {}

//---------------------Registration---------------------

VarId SolverBase::makeIntVar(Int initValue, Int lowerBound, Int upperBound) {
  if (!_isOpen) {
    throw SolverClosedException("Cannot make IntVar when store is closed.");
  }
  VarId newId =
      _store.createIntVar(_currentTimestamp, initValue, lowerBound, upperBound);
  registerVar(newId);
  return newId;
}
}  // namespace atlantis::propagation

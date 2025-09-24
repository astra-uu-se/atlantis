#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

SolverBase::SolverBase()
    : _currentTimestamp(NULL_TIMESTAMP + 1), _isOpen(false) {}

//--------------------- Variable ---------------------
void SolverBase::updateValues(const std::unordered_map<VarId, Int>&  values) {
  for (auto& [varId, value] : values) {
    updateValue(varId, value);
  }
}

//---------------------Registration---------------------

VarViewId SolverBase::makeIntVar(Int initValue, Int lowerBound,
                                 Int upperBound) {
  if (!_isOpen) {
    throw SolverClosedException("Cannot make IntVar when store is closed.");
  }
  const VarViewId newId =
      _store.createIntVar(_currentTimestamp, initValue, lowerBound, upperBound);
  registerVar(VarId(newId));
  return newId;
}
}  // namespace atlantis::propagation

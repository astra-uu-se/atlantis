#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

void SolverBase::incValue(const VarId id, const Int val) {
  incValue(_currentTimestamp, id, val);
}

void SolverBase::updateValue(const VarId id, const Int val) {
  updateValue(_currentTimestamp, id, val);
}

SolverBase::SolverBase()
    : _currentTimestamp(NULL_TIMESTAMP + 1), _isOpen(false) {}

//--------------------- Variable ---------------------
void SolverBase::updateSearchValues(
    const std::vector<std::pair<VarId, Int>>& values) {
  for (auto& [varId, value] : values) {
    updateValue(varId, value);
  }
}

//---------------------Registration---------------------

VarViewId SolverBase::makeIntVar(const Int initValue, const Int lowerBound,
                                 const Int upperBound) {
  if (!_isOpen) {
    throw SolverClosedException("Cannot make IntVar when store is closed.");
  }
  const VarViewId newId =
      _store.createIntVar(_currentTimestamp, initValue, lowerBound, upperBound);
  registerVar(VarId{newId});
  return newId;
}
}  // namespace atlantis::propagation

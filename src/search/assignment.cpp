#include "search/assignment.hpp"

Int search::Assignment::value(VarId var) const noexcept {
  return _engine.getCommittedValue(var);
}

bool search::Assignment::satisfiesConstraints() const noexcept {
  return _engine.getCommittedValue(_violation) == 0;
}

search::Cost search::Assignment::cost() const noexcept {
  return {_engine.getCommittedValue(_violation),
          _engine.getCommittedValue(_objective)};
}

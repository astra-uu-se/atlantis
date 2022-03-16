#include "search/assignment.hpp"

Int search::Assignment::value(VarId var) const noexcept {
  return _engine.committedValue(var);
}

bool search::Assignment::satisfiesConstraints() const noexcept {
  return _engine.committedValue(_violation) == 0;
}

search::Cost search::Assignment::cost() const noexcept {
  return {_engine.committedValue(_violation),
          _engine.committedValue(_objective)};
}

#include "search/assignment.hpp"

Int search::Assignment::value(VarId var) {
  return _engine.getCommittedValue(var);
}

void search::Assignment::assign(
    const std::function<void(AssignmentModification &)> &modificationFunc) {
  _engine.beginMove();
  AssignmentModification modifications(_engine);
  modificationFunc(modifications);
  _engine.endMove();

  _engine.beginCommit();
  _engine.query(_objective);
  _engine.endCommit();
}

bool search::Assignment::satisfiesConstraints() {
  return _engine.getCommittedValue(_violation) == 0;
}

#include "search/assignment.hpp"

Int search::Assignment::value(VarId var) const noexcept {
  return _engine.getCommittedValue(var);
}

void search::Assignment::assign(
    const std::function<void(AssignmentModification &)> &modificationFunc) {
  move(modificationFunc);

  _engine.beginCommit();
  _engine.query(_objective);
  _engine.endCommit();
}

search::Cost search::Assignment::probe(
    const std::function<void(AssignmentModification &)> &modificationFunc) {
  move(modificationFunc);

  _engine.beginQuery();
  _engine.query(_objective);
  _engine.query(_violation);
  _engine.endQuery();

  return {_engine.getNewValue(_violation), _engine.getNewValue(_objective)};
}

bool search::Assignment::satisfiesConstraints() const noexcept {
  return _engine.getCommittedValue(_violation) == 0;
}

search::Cost search::Assignment::cost() const noexcept {
  return {_engine.getCommittedValue(_violation),
          _engine.getCommittedValue(_objective)};
}

void search::Assignment::move(
    const std::function<void(AssignmentModification &)> &modificationFunc) {
  _engine.beginMove();
  AssignmentModification modifications(_engine);
  modificationFunc(modifications);
  _engine.endMove();
}

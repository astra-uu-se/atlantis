#include "search/assignment.hpp"

search::Objective search::Assignment::probeMove(Move& m) {
  m.apply(_engine);

  _engine.beginQuery();
  _engine.query(_totalViolations);
  //  _engine.query(_objectiveValue);
  _engine.endQuery();

  return {_engine.getNewValue(_totalViolations)/*,
          _engine.getNewValue(_objectiveValue)*/};
}

void search::Assignment::commitMove(Move& m) {
  m.apply(_engine);

  commit();

  _objective = Objective(_engine.getCommittedValue(_totalViolations)/*,
                         _engine.getCommittedValue(_objectiveValue)*/);
}

void search::Assignment::commit() {
  _engine.beginCommit();
  _engine.query(_totalViolations);
  //  _engine.query(_objectiveValue);
  _engine.endCommit();
}

void search::Assignment::initialise(
    search::neighbourhoods::MaxViolatingNeighbourhood& neighbourhood) {
  _engine.beginMove();
  neighbourhood.initialise(_engine);
  _engine.endMove();

  commit();
}

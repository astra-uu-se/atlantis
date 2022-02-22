#pragma once

#include "core/propagationEngine.hpp"
#include "move.hpp"
#include "neighbourhoods/maxViolating.hpp"
#include "objective.hpp"

namespace search {

class Assignment {
 private:
  PropagationEngine& _engine;
  VarId _totalViolations;
  //  VarId _objectiveValue;
  Objective _objective;

 public:
  Assignment(PropagationEngine& engine, VarId totalViolations/*,
             VarId objectiveValue*/)
      : _engine(engine),
        _totalViolations(totalViolations),
//        _objectiveValue(objectiveValue),
        _objective(Objective(engine.getCommittedValue(totalViolations)/*,
                             engine.getCommittedValue(objectiveValue)*/)) {}

  Objective probeMove(Move& m);
  void commitMove(Move& m);

  void initialise(neighbourhoods::MaxViolatingNeighbourhood& neighbourhood);

  Objective objective() { return _objective; }

  PropagationEngine& engine() { return _engine; }

  [[nodiscard]] bool satisfiesConstraints() const {
    return _engine.getCommittedValue(_totalViolations) == 0;
  }

 private:
  void commit();
};

}  // namespace search
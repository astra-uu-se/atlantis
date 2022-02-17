#pragma once

#include "Objective.hpp"
#include "core/propagationEngine.hpp"
#include "move.hpp"

namespace search {

class Assignment {
 private:
  PropagationEngine& _engine;
  VarId _totalViolations;
  VarId _objectiveValue;
  Objective _objective;

 public:
  Assignment(PropagationEngine& engine, VarId totalViolations,
             VarId objectiveValue)
      : _engine(engine),
        _totalViolations(totalViolations),
        _objectiveValue(objectiveValue),
        _objective(Objective(engine.getCommittedValue(totalViolations),
                             engine.getCommittedValue(objectiveValue))) {}

  Objective probeMove(Move& m);
  void commitMove(Move& m);

  Objective objective() { return _objective; }

  PropagationEngine& engine() { return _engine; }

  [[nodiscard]] bool satisfiesConstraints() const {
    return _totalViolations == 0;
  }
};

}  // namespace search
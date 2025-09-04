#pragma once

#include <vector>

#include "atlantis/propagation/types.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::propagation {
class Solver;
}

namespace atlantis::search {

namespace neighborhoods {
class Neighborhood;
}

class Assignment {
  propagation::Solver& _solver;
  neighborhoods::Neighborhood& _neighborhood;
  propagation::VarViewId _violation{propagation::NULL_ID};
  propagation::VarViewId _objective{propagation::NULL_ID};
  ObjectiveDirection _objectiveDirection;
  Int _objectiveOptimalValue;

 public:
  explicit Assignment(propagation::Solver& solver,
                      neighborhoods::Neighborhood& neighborhood,
                      propagation::VarViewId violation,
                      propagation::VarViewId objective,
                      ObjectiveDirection objectiveDirection,
                      Int objectiveOptimalValue);

  virtual ~Assignment() = default;

  virtual Cost initialize(RandomProvider&);

  virtual Cost performProbe(RandomProvider&);

  virtual void commitLastProbe();

  /**
   * Get the current value of a variable in the assignment.
   */
  [[nodiscard]] virtual Int currentValue(propagation::VarViewId) const;

  /**
   * Get the committed of a variable in the assignment.
   */
  [[nodiscard]] virtual Int committedValue(propagation::VarViewId) const;

  /**
   * @return True if the current assignment satisfies all the constraints, false
   * otherwise.
   */
  [[nodiscard]] virtual bool satisfiesConstraints() const;

  [[nodiscard]] virtual bool objectiveIsOptimal() const;

  virtual void set(propagation::VarId searchVarId, Int val);

  [[nodiscard]] virtual const std::vector<propagation::VarId>& searchVars()
      const;

  [[nodiscard]] virtual Timestamp currentTimestamp() const;

  [[nodiscard]] virtual ObjectiveDirection objectiveDirection() const;

  [[nodiscard]] virtual Cost getCost() const;
};

}  // namespace atlantis::search

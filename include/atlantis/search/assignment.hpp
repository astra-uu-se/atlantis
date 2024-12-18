#pragma once

#include <vector>

#include "atlantis/propagation/types.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/search/iAssignment.hpp"

namespace atlantis::propagation {
class Solver;
}

namespace atlantis::search {

namespace neighborhoods {
class Neighborhood;
}

class Assignment : public virtual IAssignment {
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

  Cost initialize(RandomProvider&) override;

  Cost performProbe(RandomProvider&) override;

  void commitLastProbe() override;

  /**
   * Get the current value of a variable in the assignment.
   */
  [[nodiscard]] Int currentValue(propagation::VarViewId) const override;

  /**
   * Get the committed of a variable in the assignment.
   */
  [[nodiscard]] Int committedValue(propagation::VarViewId) const override;

  /**
   * @return True if the current assignment satisfies all the constraints, false
   * otherwise.
   */
  [[nodiscard]] bool satisfiesConstraints() const override;

  [[nodiscard]] bool objectiveIsOptimal() const override;

  void set(propagation::VarId searchVarId, Int val) override;

  [[nodiscard]] const std::vector<propagation::VarId>& searchVars()
      const override;

  [[nodiscard]] Timestamp currentTimestamp() const override;

  [[nodiscard]] ObjectiveDirection objectiveDirection() const override;
};

}  // namespace atlantis::search

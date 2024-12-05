#pragma once

#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"

namespace atlantis::search {

class Assignment : public virtual IAssignment {
 private:
  propagation::Solver& _solver;
  neighbourhoods::Neighbourhood& _neighbourhood;
  propagation::VarViewId _violation{propagation::NULL_ID};
  propagation::VarViewId _objective{propagation::NULL_ID};
  ObjectiveDirection _objectiveDirection;
  Int _objectiveOptimalValue;

 public:
  explicit Assignment(propagation::Solver& solver,
                      neighbourhoods::Neighbourhood& neighbourhood,
                      propagation::VarViewId violation,
                      propagation::VarViewId objective,
                      ObjectiveDirection objectiveDirection,
                      Int objectiveOptimalValue);

  Cost initialise(RandomProvider&) override;

  Cost performProbe(RandomProvider&) override;

  void commitLastProbe() override;

  /**
   * Get the current value of a variable in the assignment.
   *
   * @param var The variable for which to query the value.
   * @return The value of @p var.
   */
  [[nodiscard]] Int currentValue(propagation::VarViewId) const override;

  /**
   * Get the committed of a variable in the assignment.
   *
   * @param var The variable for which to query the value.
   * @return The value of @p var.
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

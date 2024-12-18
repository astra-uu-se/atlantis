#pragma once

#include <vector>

#include "atlantis/propagation/types.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

class RandomProvider;

class IAssignment {
 public:
  virtual ~IAssignment() = default;

  virtual Cost initialize(RandomProvider&) = 0;

  virtual Cost performProbe(RandomProvider&) = 0;

  virtual void commitLastProbe() = 0;
  /**
   * Get the current value of a variable in the assignment.
   *
   * @param var The variable for which to query the value.
   * @return The value of @p var.
   */
  [[nodiscard]] virtual Int currentValue(propagation::VarViewId var) const = 0;

  /**
   * Get the committed of a variable in the assignment.
   *
   * @param var The variable for which to query the value.
   * @return The value of @p var.
   */
  [[nodiscard]] virtual Int committedValue(
      propagation::VarViewId var) const = 0;

  /**
   * @return True if the current assignment satisfies all the constraints, false
   * otherwise.
   */
  [[nodiscard]] virtual bool satisfiesConstraints() const = 0;

  [[nodiscard]] virtual bool objectiveIsOptimal() const = 0;

  virtual void set(propagation::VarId searchVarId, Int val) = 0;

  [[nodiscard]] virtual const std::vector<propagation::VarId>& searchVars()
      const = 0;

  [[nodiscard]] virtual Timestamp currentTimestamp() const = 0;

  [[nodiscard]] virtual ObjectiveDirection objectiveDirection() const = 0;
};

}  // namespace atlantis::search

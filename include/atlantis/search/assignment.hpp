#pragma once

#include <unordered_map>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::propagation {
class Solver;
}

namespace atlantis::search {
class SavedAssignment;

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

  ~Assignment() = default;

  Cost initialize(RandomProvider&);

  Cost performProbe(RandomProvider&);

  void commitLastProbe();

  /**
   * Get the current value of a variable in the assignment.
   */
  [[nodiscard]] Int currentValue(propagation::VarViewId) const;

  [[nodiscard]] std::unordered_map<propagation::VarId, Int> currentValues() const;

  /**
   * Get the committed value of a variable in the assignment.
   */
  [[nodiscard]] Int committedValue(propagation::VarViewId) const;

  /**
   * @return True if the current assignment satisfies all the constraints, false
   * otherwise.
   */
  [[nodiscard]] bool satisfiesConstraints() const;

  [[nodiscard]] bool objectiveIsOptimal() const;

  void set(propagation::VarId searchVarId, Int val);

  [[nodiscard]] const std::vector<propagation::VarId>& searchVars()
      const;

  [[nodiscard]] Timestamp currentTimestamp() const;

  [[nodiscard]] ObjectiveDirection objectiveDirection() const;

  [[nodiscard]] Cost getCost() const;

  void setAssignment(SavedAssignment saved) const;
};

}  // namespace atlantis::search

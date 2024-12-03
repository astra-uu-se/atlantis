#pragma once

#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/search/cost.hpp"

namespace atlantis::search {

class Assignment {
 private:
  propagation::Solver& _solver;
  propagation::VarViewId _violation{propagation::NULL_ID};
  propagation::VarViewId _objective{propagation::NULL_ID};
  propagation::ObjectiveDirection _objectiveDirection;
  Int _objectiveOptimalValue;

 public:
  explicit Assignment(propagation::Solver& solver,
                      propagation::VarViewId violation,
                      propagation::VarViewId objective,
                      propagation::ObjectiveDirection objectiveDirection,
                      Int objectiveOptimalValue);

  /**
   * Assign values to the variables in the assignment. This is supplied a
   * callback, which receives a @p AssignmentModification instance, through
   * which the variables can be changed. For example:
   *
   *     Assignment assignment;
   *     assignment.assign([&](auto& modifications) {
   *       modifications.set(a, 1);
   *       modifications.set(b, 2);
   *     });
   *
   * @param modificationFunc The callback which sets the variables and their
   * new values.
   */
  template <typename Callback>
  void assign(Callback modificationFunc) {
    move(modificationFunc);

    _solver.beginCommit();
    _solver.query(_violation);
    _solver.query(_objective);
    _solver.endCommit();
  }

  /**
   * Probe the cost of a modification to the assignment. Works similarly to
   * assign(), but does not commit the modification.
   *
   * @param modificationFunc The callback which sets the variables to their
   * new values for the probe.
   * @return The cost of the assignment if the altered values were committed.
   */
  template <typename Callback>
  Cost probe(Callback modificationFunc) const {
    move(modificationFunc);

    _solver.beginProbe();
    _solver.query(_objective);
    _solver.query(_violation);
    _solver.endProbe();

    return {_solver.currentValue(_violation), _solver.currentValue(_objective),
            _objectiveDirection};
  }

  /**
   * Get the value of a variable in the current assignment.
   *
   * @param var The variable for which to query the value.
   * @return The value of @p var.
   */
  [[nodiscard]] Int value(propagation::VarViewId var) const noexcept;

  /**
   * @return True if the current assignment satisfies all the constraints, false
   * otherwise.
   */
  [[nodiscard]] bool satisfiesConstraints() const noexcept;

  [[nodiscard]] bool objectiveIsOptimal() const noexcept;

  inline void set(propagation::VarId searchVarId, Int val) {
    _solver.setValue(searchVarId, val);
  }

  /**
   * @return The cost of the current assignment.
   */
  [[nodiscard]] Cost cost() const noexcept;

  [[nodiscard]] const std::vector<propagation::VarId>& searchVars()
      const noexcept {
    return _solver.searchVars();
  }

 private:
  template <typename Callback>
  void move(Callback modificationFunc) const {
    _solver.beginMove();
    modificationFunc(*this);
    _solver.endMove();
  }
};

}  // namespace atlantis::search

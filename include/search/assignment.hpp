#pragma once

#include <functional>

#include "propagation/propagationEngine.hpp"
#include "search/cost.hpp"

namespace atlantis::search {

class AssignmentModifier {
 private:
  propagation::PropagationEngine& _engine;

 public:
  explicit AssignmentModifier(propagation::PropagationEngine& engine)
      : _engine(engine) {
    assert(engine.isMoving());
  }

  /**
   * Assign a value to a variable. Overrides any modifications previously made
   * to @p var.
   *
   * @param var The variable to assign.
   * @param value The value to assign to the variable.
   */
  void set(propagation::VarId var, Int value) { _engine.setValue(var, value); }
};

class Assignment {
 private:
  propagation::PropagationEngine& _engine;
  std::vector<propagation::VarId> _searchVariables{};
  propagation::VarId _violation;
  propagation::VarId _objective;
  propagation::ObjectiveDirection _objectiveDirection;

 public:
  explicit Assignment(propagation::PropagationEngine& engine,
                      propagation::VarId violation,
                      propagation::VarId objective,
                      propagation::ObjectiveDirection objectiveDirection);

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

    _engine.beginCommit();
    _engine.query(_violation);
    _engine.query(_objective);
    _engine.endCommit();
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

    _engine.beginProbe();
    _engine.query(_objective);
    _engine.query(_violation);
    _engine.endProbe();

    return {_engine.currentValue(_violation), _engine.currentValue(_objective),
            _objectiveDirection};
  }

  /**
   * Get the value of a variable in the current assignment.
   *
   * @param var The variable for which to query the value.
   * @return The value of @p var.
   */
  [[nodiscard]] Int value(propagation::VarId var) const noexcept;

  /**
   * @return True if the current assignment satisfies all the constraints, false
   * otherwise.
   */
  [[nodiscard]] bool satisfiesConstraints() const noexcept;

  /**
   * @return The cost of the current assignment.
   */
  [[nodiscard]] Cost cost() const noexcept;

  [[nodiscard]] const std::vector<propagation::VarId>& searchVariables()
      const noexcept {
    return _searchVariables;
  }

 private:
  template <typename Callback>
  void move(Callback modificationFunc) const {
    _engine.beginMove();
    AssignmentModifier modifications(_engine);
    modificationFunc(modifications);
    _engine.endMove();
  }
};

}  // namespace atlantis::search
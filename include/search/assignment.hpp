#pragma once

#include <functional>

#include "core/propagationEngine.hpp"
#include "cost.hpp"

namespace search {

class AssignmentModification {
 private:
  PropagationEngine& _engine;

 public:
  explicit AssignmentModification(PropagationEngine& engine) : _engine(engine) {
    assert(engine.isMoving());
  }

  /**
   * Assign a value to a variable. Overrides any modifications previously made
   * to @p var.
   *
   * @param var The variable to assign.
   * @param value The value to assign to the variable.
   */
  void set(VarId var, Int value) { _engine.setValue(var, value); }
};

class Assignment {
 private:
  PropagationEngine& _engine;
  VarId& _violation;
  VarId& _objective;

 public:
  explicit Assignment(PropagationEngine& engine, VarId& violation,
                      VarId& objective)
      : _engine(engine), _violation(violation), _objective(objective) {}

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
  Cost probe(Callback modificationFunc) {
    move(modificationFunc);

    _engine.beginQuery();
    _engine.query(_objective);
    _engine.query(_violation);
    _engine.endQuery();

    return {_engine.getNewValue(_violation), _engine.getNewValue(_objective)};
  }

  /**
   * Get the value of a variable in the current assignment.
   *
   * @param var The variable for which to query the value.
   * @return The value of @p var.
   */
  [[nodiscard]] Int value(VarId var) const noexcept;

  /**
   * @return True if the current assignment satisfies all the constraints, false
   * otherwise.
   */
  [[nodiscard]] bool satisfiesConstraints() const noexcept;

  /**
   * @return The cost of the current assignment.
   */
  [[nodiscard]] Cost cost() const noexcept;

 private:
  template <typename Callback>
  void move(Callback modificationFunc) {
    _engine.beginMove();
    AssignmentModification modifications(_engine);
    modificationFunc(modifications);
    _engine.endMove();
  }
};

}  // namespace search
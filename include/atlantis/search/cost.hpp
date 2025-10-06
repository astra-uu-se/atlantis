#pragma once

#include <string>

#include "atlantis/types.hpp"

namespace atlantis::search {

class Assignment;

class Cost {
  Int _violationDegree;
  Int _objective;
  Int _objectiveWeightSign;

 public:
  Cost(Int violationDegree, Int objective, ObjectiveDirection direction);

  Cost(const Assignment&);

  /**
   * @return True if this cost has no violated constraints.
   */
  [[nodiscard]] bool satisfiesConstraints() const noexcept {
    return _violationDegree == 0;
  }

  /**
   * Evaluate the value of this cost, given weights for the components of the
   * cost.
   *
   * @param violationWeight The weight of the violation.
   * @param objectiveWeight The weight of the objective value.
   * @return The scalar cost value, given the component weights.
   */
  [[nodiscard]] Int evaluate(UInt violationWeight,
                             UInt objectiveWeight) const noexcept;

  [[nodiscard]] bool isBetterThan(const Cost &other) const;

  [[nodiscard]] bool isStrictlyBetterThan(const Cost &other) const;

  [[nodiscard]] std::string toString() const;

  [[nodiscard]] Int getObjective() const { return _objective; }

  [[nodiscard]] Int getViolation() const { return _violationDegree; }

  void set(Int violationDegree, Int objective) {
    _violationDegree = violationDegree;
    _objective = objective;
  }
};

}  // namespace atlantis::search

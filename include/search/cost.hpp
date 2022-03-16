#pragma once

#include "core/types.hpp"

namespace search {

class Cost {
 private:
  Int _violationDegree;
  Int _objective;

 public:
  Cost(Int violationDegree, Int objective)
      : _violationDegree(violationDegree), _objective(objective) {}

  /**
   * @return True if this cost has no violated constraints.
   */
  [[nodiscard]] inline bool satisfiesConstraints() const noexcept {
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
  [[nodiscard]] inline Int evaluate(Int violationWeight,
                                    Int objectiveWeight) const noexcept {
    return violationWeight * _violationDegree +
           objectiveWeight * _objective;
  }
};

}  // namespace search
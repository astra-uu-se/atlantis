#pragma once

#include <string>

#include "propagation/types.hpp"
#include "types.hpp"

namespace atlantis::search {

class Cost {
 private:
  Int _violationDegree;
  Int _objective;
  Int _objectiveWeightSign;

 public:
  Cost(Int violationDegree, Int objective,
       propagation::ObjectiveDirection direction);

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
  [[nodiscard]] Int evaluate(UInt violationWeight,
                             UInt objectiveWeight) const noexcept;

  [[nodiscard]] Int violationDegree() const noexcept {
    return _violationDegree;
  }

  [[nodiscard]] Int objectiveValue() const noexcept { return _objective; }

  [[nodiscard]] std::string toString() const;
};

}  // namespace atlantis::search
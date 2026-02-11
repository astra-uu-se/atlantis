#pragma once

#include <string>
#include <optional>

#include "atlantis/types.hpp"

namespace atlantis::search {

class Assignment;

class Cost {
   std::optional<Int> _violation;
   std::optional<Int> _objective;

 public:
  explicit Cost();
  explicit Cost(Int violationDegree);
  explicit Cost(bool hasViolation, ObjectiveDirection direction);
  explicit Cost(Int objective, bool isMinimization);
  Cost(Int violationDegree, Int objective, bool isMinimization);

  explicit Cost(const Assignment&);


  /**
   * @return True if this cost has no violated constraints.
   */
  [[nodiscard]] bool satisfiesConstraints() const noexcept;

  [[nodiscard]] std::string toString() const;

  [[nodiscard]] bool hasViolation() const;

  [[nodiscard]] bool hasObjective() const;

  [[nodiscard]] Int objective() const;

  [[nodiscard]] Int violation() const;

  [[nodiscard]] bool operator<(const Cost& other) const noexcept;

  [[nodiscard]] bool operator<=(const Cost& other) const noexcept;

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
};

}  // namespace atlantis::search

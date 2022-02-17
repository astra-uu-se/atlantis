#pragma once

#include "core/types.hpp"

namespace search {

class Objective {
 private:
  Int _violations;
  Int _modelObjective;

 public:
  Objective(Int violations, Int modelObjective)
      : _violations(violations), _modelObjective(modelObjective) {}

  [[nodiscard]] Int evaluate(Int violationsWeight,
                             Int modelObjectiveWeight) const {
    return violationsWeight * _violations +
           modelObjectiveWeight * _modelObjective;
  }
};

}  // namespace search
#include "atlantis/search/cost.hpp"

namespace atlantis::search {

Cost::Cost(Int violationDegree, Int objective, ObjectiveDirection direction)
    : _violationDegree(violationDegree),
      _objective(objective),
      _objectiveWeightSign(static_cast<int>(direction)) {}

Int Cost::evaluate(UInt violationWeight, UInt objectiveWeight) const noexcept {
  return static_cast<Int>(violationWeight) * _violationDegree +
         static_cast<Int>(objectiveWeight) * _objectiveWeightSign * _objective;
}

bool Cost::isBetterThan(const Cost &other) const {
  if (evaluate(1, 0) != 0 || other.evaluate(1, 0) != 0) {
    return evaluate(1, 0) <= other.evaluate(1, 0);
  }

  return evaluate(0, 1) <= other.evaluate(0, 1);
}

bool Cost::isStrictlyBetterThan(const Cost &other) const {
  if (evaluate(1, 0) != 0 || other.evaluate(1, 0) != 0) {
    return evaluate(1, 0) < other.evaluate(1, 0);
  }

  return evaluate(0, 1) < other.evaluate(0, 1);
}

std::string Cost::toString() const {
  return '<' + std::to_string(_violationDegree) + ", " +
         std::to_string(_objective) + '>';
}

}  // namespace atlantis::search

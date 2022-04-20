#include "search/cost.hpp"

search::Cost::Cost(Int violationDegree, Int objective,
                   search::Cost::ObjectiveDirection direction)
    : _violationDegree(violationDegree),
      _objective(objective),
      _objectiveWeightSign(static_cast<int>(direction)) {}

Int search::Cost::evaluate(UInt violationWeight,
                           UInt objectiveWeight) const noexcept {
  return violationWeight * _violationDegree +
         objectiveWeight * _objectiveWeightSign * _objective;
}

std::string search::Cost::toString() const {
  return '<' + std::to_string(_violationDegree) + ", " +
         std::to_string(_objective) + '>';
}

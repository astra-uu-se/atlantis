#include "search/cost.hpp"

namespace atlantis::search {

Cost::Cost(Int violationDegree, Int objective,
           propagation::ObjectiveDirection direction)
    : _violationDegree(violationDegree),
      _objective(objective),
      _objectiveWeightSign(static_cast<int>(direction)) {}

Int Cost::evaluate(UInt violationWeight, UInt objectiveWeight) const noexcept {
  return violationWeight * _violationDegree +
         objectiveWeight * _objectiveWeightSign * _objective;
}

std::string Cost::toString() const {
  return '<' + std::to_string(_violationDegree) + ", " +
         std::to_string(_objective) + '>';
}

}  // namespace atlantis::search
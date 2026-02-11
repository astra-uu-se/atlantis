
#include "atlantis/search/cost.hpp"
#include "atlantis/utils/overflow.hpp"

#include <limits>


#include "atlantis/search/assignment.hpp"

namespace atlantis::search {

inline std::optional<Int> getViol(const Assignment& assignment) {
  if (!assignment.hasViolation()) {
    return std::nullopt;
  }
  return assignment.currentViolation();
}

inline Int getObj(const Int obj, const bool minimize) {
  if (minimize) {
    return obj;
  }
  Int res = std::numeric_limits<Int>::max();
  if (mul_overflow(Int{-1}, obj, res)) {
    return std::numeric_limits<Int>::max();
  }
  return res;
}

inline std::optional<Int> getObj(const Int obj, const ObjectiveDirection dir) {
  if (dir == ObjectiveDirection::NONE) {
    return std::nullopt;
  }
  return {getObj(obj, dir == ObjectiveDirection::MINIMIZE)};
}

inline std::optional<Int> getObj(const ObjectiveDirection dir) {
  if (dir == ObjectiveDirection::NONE) {
    return std::nullopt;
  }
  return {std::numeric_limits<Int>::max()};
}

inline std::optional<Int> getObj(const Assignment& assignment) {
  if (!assignment.hasObjective() || assignment.objectiveDirection() == ObjectiveDirection::NONE) {
    return std::nullopt;
  }
  return {getObj(assignment.currentObjective(), assignment.objectiveDirection() == ObjectiveDirection::MINIMIZE)};
}

Cost::Cost() : _violation(std::nullopt), _objective(std::nullopt) {}

Cost::Cost(Int violationDegree) : _violation(violationDegree), _objective(std::nullopt) {}

Cost::Cost(const bool hasViolation, ObjectiveDirection direction) : _violation(hasViolation ? std::optional<Int>{std::numeric_limits<Int>::max()} : std::optional<Int>{std::nullopt}), _objective(getObj(direction)) {}

Cost::Cost(const Int objective, const bool isMinimization) : _violation(std::nullopt), _objective(getObj(objective, isMinimization)) {}

Cost::Cost(const Int violationDegree, const Int objective, const bool isMinimization) : _violation(violationDegree), _objective(getObj(objective, isMinimization)) {}

Cost::Cost(const Assignment& assignment) : _violation(getViol(assignment)), _objective(getObj(assignment)) {}

bool Cost::satisfiesConstraints() const noexcept {
  return !_violation.has_value() || (*_violation == 0);
}

Int Cost::evaluate(const UInt violationWeight,
                   const UInt objectiveWeight) const noexcept {
  if (_violation.has_value() && _objective.has_value()) {
    Int violProd = 0;
    Int objProd = 0;
    Int sum = 0;
    if (!mul_overflow(static_cast<Int>(violationWeight),
                      *_violation, violProd) &&
        !mul_overflow(*_objective, static_cast<Int>(objectiveWeight), objProd) &&
        !add_overflow(violProd, objProd, sum)) {
      return sum;
    }
    return std::numeric_limits<Int>::max();
  }
  if (_objective.has_value()) {
    return *_objective;
  }
  if (_violation.has_value()) {
    return *_violation;
  }
  return std::numeric_limits<Int>::max();

}

bool Cost::operator<(const Cost& other) const noexcept {
  if (!_violation.has_value() && !_objective.has_value()) {
    return false;
  }
  if (!other._violation.has_value() && !other._objective.has_value()) {
    return true;
  }
  if (_violation.has_value() != other._violation.has_value() ||
    _objective.has_value() != other._objective.has_value()) {
    return false;
  }
  if (_violation.has_value() &&
      other._violation.has_value() &&
      _objective.has_value() &&
      other._objective.has_value()) {
    if (*_violation !=
        other._violation.value()) {
      return _violation <
             other._violation.value();
    }
    return *_objective < other._objective.value();
  }
  if (_objective.has_value() &&
      other._objective.has_value()) {
    return *_objective <
                     other._objective.value();
  }
  if (_violation.has_value() &&
      other._violation.has_value()) {
    return _violation < other._violation.value();
  }
  return false;
}

bool Cost::operator<=(const Cost& other) const noexcept {
  if (!_violation.has_value() && !_objective.has_value()) {
    return !other._violation.has_value() && !other._objective.has_value();
  }
  if (!other._violation.has_value() && !other._objective.has_value()) {
    return true;
  }
  if (_violation.has_value() != other._violation.has_value() ||
    _objective.has_value() != other._objective.has_value()) {
    return false;
  }
  if (_violation.has_value() &&
      other._violation.has_value() &&
      _objective.has_value() &&
      other._objective.has_value()) {
    if (*_violation !=
        other._violation.value()) {
      return _violation <
             other._violation.value();
        }
    return *_objective <= other._objective.value();
      }
  if (_objective.has_value() &&
      other._objective.has_value()) {
    return *_objective <=
                     other._objective.value();
      }
  if (_violation.has_value() &&
      other._violation.has_value()) {
    return _violation <= other._violation.value();
      }
  return false;
}

std::string Cost::toString() const {
  if (_violation.has_value() && _objective.has_value()) {
    return '<' + std::to_string(*_violation) + ", " +
           std::to_string(*_objective) + '>';
  }
  if (_objective.has_value()) {
    return "<-, " + std::to_string(*_objective) +
           '>';
  }
  if (_violation.has_value()) {
    return '<' + std::to_string(*_violation) + ", ->";
  }
  return "<-, ->";
}

bool Cost::hasViolation() const {
  return _violation.has_value();
}

bool Cost::hasObjective() const { return _objective.has_value(); }

Int Cost::objective() const {
  return _objective.value_or(0);
}
Int Cost::violation() const {
  return _violation.value_or(0);
}

}  // namespace atlantis::search

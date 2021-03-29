#pragma once
#include "../constraint.hpp"

class ArrayConstraint : public SimpleConstraint {
 public:
  ArrayConstraint(ConstraintBox constraintBox)
      : SimpleConstraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  ArrayVariable* _x;
};
class ArrayIntMaximum : public ArrayConstraint {
 public:
  ArrayIntMaximum(ConstraintBox constraintBox)
      : ArrayConstraint(constraintBox) {}
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
};
class ArrayIntMinimum : public ArrayConstraint {
 public:
  ArrayIntMinimum(ConstraintBox constraintBox)
      : ArrayConstraint(constraintBox) {}
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
};

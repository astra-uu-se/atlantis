#pragma once
#include "../constraint.hpp"

class TwoSVarConstraint : public SimpleConstraint {
 public:
  TwoSVarConstraint(ConstraintBox constraintBox)
      : SimpleConstraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
};
class IntAbs : public TwoSVarConstraint {
 public:
  IntAbs(ConstraintBox constraintBox) : TwoSVarConstraint(constraintBox) {}
  std::pair<Int, Int> calculateDomain(Variable* var) override;
};
class IntEq : public TwoSVarConstraint {
 public:
  IntEq(ConstraintBox constraintBox) : TwoSVarConstraint(constraintBox) {
    _uniqueTarget = false;
  }
  void configureVariables() override;
  std::pair<Int, Int> calculateDomain(Variable* var) override;
};

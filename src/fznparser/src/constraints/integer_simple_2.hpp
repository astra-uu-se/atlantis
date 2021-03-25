#pragma once
#include "../constraint.hpp"

class TwoSVarConstraint : public Constraint {
 public:
  TwoSVarConstraint(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  // bool imposeDomain(Variable* variable) override;
  // bool refreshDomain() override;

  // virtual std::pair<Int, Int> calculateDomain() = 0;
  std::shared_ptr<IntDomain> _outputDomain;
};
class IntAbs : public TwoSVarConstraint {
 public:
  IntAbs(ConstraintBox constraintBox) : TwoSVarConstraint(constraintBox) {}
};

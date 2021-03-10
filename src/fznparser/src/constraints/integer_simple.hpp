#pragma once
#include "../constraint.hpp"

class ThreeSVarConstraint : public Constraint {
 public:
  ThreeSVarConstraint(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _outputDomain = std::make_shared<IntDomain>();
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  std::shared_ptr<IntDomain> _outputDomain;
};
/* int_div(var int: a, var int: b, var int: c)
** Defines: a
** Depends: b, c
*/
class IntDiv : public ThreeSVarConstraint {
 public:
  IntDiv(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {}
  void configureVariables() override;
  bool imposeDomain(Variable* variable) override;
  bool refreshDomain() override;
};
/* int_plus(var int: a, var int: b, var int: c)
** Defines: any
*/
class IntPlus : public ThreeSVarConstraint {
 public:
  IntPlus(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {
    _uniqueTarget = false;
  }
  void configureVariables() override;
  bool imposeDomain(Variable* variable) override;
  bool refreshDomain() override;
  std::pair<Int, Int> getBounds(Variable* variable);
};
class TwoSVarConstraint : public Constraint {
 public:
  TwoSVarConstraint(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
};
class IntAbs : public TwoSVarConstraint {
 public:
  IntAbs(ConstraintBox constraintBox) : TwoSVarConstraint(constraintBox) {}
};

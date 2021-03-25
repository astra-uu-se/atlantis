#pragma once
#include "../constraint.hpp"

class ThreeSVarConstraint : public Constraint {
 public:
  ThreeSVarConstraint(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _outputDomain = std::make_shared<IntDomain>();
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool imposeDomain(Variable* variable) override;
  bool refreshDomain() override;

  virtual std::pair<Int, Int> calculateDomain(Variable* variable) = 0;
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
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
};
/* int_max(var int: a, var int: b, var int: c)
** Defines: c
** Depends: a, b
*/
class IntMax : public ThreeSVarConstraint {
 public:
  IntMax(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {}
  void configureVariables() override;
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
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
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
};

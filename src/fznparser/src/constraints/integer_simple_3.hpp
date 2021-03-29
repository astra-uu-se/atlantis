#pragma once
#include "../constraint.hpp"

class ThreeSVarConstraint : public SimpleConstraint {
 public:
  ThreeSVarConstraint(ConstraintBox constraintBox)
      : SimpleConstraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
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
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
};
class IntMin : public ThreeSVarConstraint {
 public:
  IntMin(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {}
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
};
class IntMod : public ThreeSVarConstraint {
 public:
  IntMod(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {}
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
};
class IntPow : public ThreeSVarConstraint {
 public:
  IntPow(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {}
  std::pair<Int, Int> calculateDomain(Variable* variable) override;
};
class IntTimes : public ThreeSVarConstraint {
 public:
  IntTimes(ConstraintBox constraintBox) : ThreeSVarConstraint(constraintBox) {}
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

#pragma once
#include "../constraint.hpp"
/* global_cardinality(       array [int] of var int: x,
**                           array [int] of int: cover,
**                           array [int] of var int: counts)
** Defines: all of counts
** Depends: x
*/
class GlobalCardinality : public Constraint {
 public:
  GlobalCardinality(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  GlobalCardinality(ArrayVariable* x, ArrayVariable* cover,
                    ArrayVariable* counts);
  bool split(int index, VariableMap& variables,
             ConstraintMap& constraints) override;
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeOneWay(Variable* variable) override;
  bool canBeImplicit() override;
  void makeImplicit() override;
  void imposeDomain();

 private:
  ArrayVariable* _x;
  ArrayVariable* _cover;  // const array
  ArrayVariable* _counts;
};

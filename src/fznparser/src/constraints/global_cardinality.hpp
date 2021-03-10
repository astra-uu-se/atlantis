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
  void imposeAndPropagate(Variable* variable) override;
  void refreshAndPropagate(std::set<Constraint*>& visited) override;
  std::pair<Int, Int> getBounds(Int n);
  void initDomains();

 private:
  std::vector<std::shared_ptr<IntDomain>> _outputDomains;
  ArrayVariable* _x;
  ArrayVariable* _cover;  // const array
  ArrayVariable* _counts;
};

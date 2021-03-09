#pragma once
#include "../constraint.hpp"

class AllDifferent : public Constraint {
 public:
  AllDifferent(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
    _canBeOneWay = false;
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;

 private:
  ArrayVariable* _x;
};

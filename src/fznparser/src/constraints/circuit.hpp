#pragma once
#include "../constraint.hpp"

class Circuit : public Constraint {
 public:
  Circuit(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;

 private:
  ArrayVariable* _x;
};

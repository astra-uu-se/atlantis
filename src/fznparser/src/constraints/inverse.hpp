#pragma once
#include "../constraint.hpp"

class Inverse : public Constraint {
 public:
  Inverse(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;
};

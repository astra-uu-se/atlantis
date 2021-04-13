#pragma once
#include "../constraint.hpp"

class SetInReif : public Constraint {
 public:
  SetInReif(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variableMap);
  void configureVariables();
};

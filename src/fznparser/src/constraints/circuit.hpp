#pragma once

#include "../constraint.hpp"

class Circuit : public Constraint {
 public:
  Circuit(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;
  bool isFunctional() override { return false; }
  bool isPotImplicit() override { return true; }

 private:
  ArrayVariable* _x;
};

class SubCircuit : public Circuit {
 public:
  SubCircuit(ConstraintBox constraintBox) : Circuit(constraintBox) {}
};

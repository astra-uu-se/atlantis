#pragma once
#include "../constraint.hpp"

/*
 * This whole class needs to be reviewed since it was designed thinking that
 * Inverse(x, y) <-> x = reverse(y) when it should be
 * Inverse(x, y) <-> x[y[i]] = y[x[i]] = i for all i.
 */
class Inverse : public Constraint {
 public:
  Inverse(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canDefine(Variable* variable) override;
  void define(Variable* variable) override;
  void unDefine(Variable* variable) override;
  bool canBeImplicit() override;
  bool isPotImplicit() override { return true; }
  ArrayVariable* _f;
  ArrayVariable* _invf;
  std::optional<ArrayVariable*> _out;
};

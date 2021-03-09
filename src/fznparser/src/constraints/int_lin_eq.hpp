#pragma once
#include "../constraint.hpp"

class IntLinEq : public Constraint {
 public:
  IntLinEq(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
    _outputDomain = std::make_shared<IntDomain>();
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  bool canBeImplicit() override;
  void imposeDomain(Variable* variable) override;

 private:
  std::shared_ptr<IntDomain> _outputDomain;
  ArrayVariable* _as;
  ArrayVariable* _bs;
  SingleVariable* _c;
};

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
  bool imposeDomain(Variable* variable) override;
  bool refreshDomain() override;
  bool isPotImplicit() override { return canBeImplicit(); }

 private:
  static Int maxDomain(int coef, Variable* variable);
  static Int minDomain(int coef, Variable* variable);
  std::pair<Int, Int> getBounds(Variable* variable);
  std::shared_ptr<IntDomain> _outputDomain;
  std::vector<Int> _asv;
  ArrayVariable* _as;
  ArrayVariable* _bs;
  Variable* _c;
};

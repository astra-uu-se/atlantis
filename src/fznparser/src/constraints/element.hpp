#pragma once
#include "../constraint.hpp"

/*
** element(index, values, out);
*/
class Element : public Constraint {
 public:
  Element(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  std::set<Node*> getNext() override;

  // bool canDefine(Variable* variable) override;
  // void define(Variable* variable) override;
  // void unDefine(Variable* variable) override;
  // bool allTargetsFree() override;
  // bool canBeImplicit() override;
  // void makeImplicit() override;
  // void initDomains();
  // Variable* _index;
  ArrayVariable* _values;
  // Variable* _out;
  bool _allowDynamicCycles = true;
};

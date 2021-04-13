#pragma once

#include "../constraint.hpp"

/*
** element(index, values, out);
*/
class Element : public Constraint {
 public:
  Element(ConstraintBox constraintBox) : Constraint(constraintBox) {}
  void configureVariables() override;
  void loadVariables(const VariableMap& variables) override;
  ArrayVariable* _values;
};
class VarElement : public Element {
 public:
  VarElement(ConstraintBox constraintBox) : Element(constraintBox) {}
  void loadVariables(const VariableMap& variables) override;
  bool isIndexVar(Node* node);
};

class VarBoolElement : public VarElement {
 public:
  VarBoolElement(ConstraintBox constraintBox) : VarElement(constraintBox) {}
};
class BoolElement : public Element {
 public:
  BoolElement(ConstraintBox constraintBox) : Element(constraintBox) {}
};

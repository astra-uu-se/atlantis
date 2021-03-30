#pragma once
#include "../constraint.hpp"

/*
** element(index, values, out);
** At this point, the allowance of dynamic cycles simply overrides "getNext"
** such that traversal breaks here. This is not desired if traversal
** originates from the index variable. I also might want to have separate
** classes for _var_ versions.
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

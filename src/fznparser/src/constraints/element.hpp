#pragma once
#include "../constraint.hpp"

/*
** element(index, values, out);
*/
class Element : public Constraint {
 public:
  Element(ConstraintBox constraintBox) : Constraint(constraintBox) {
    _uniqueTarget = false;
  }
  void loadVariables(const VariableMap& variables) override;
  void configureVariables() override;
  std::vector<Node*> getNext() override;
  void checkAnnotations(const VariableMap& variableMap) override;
  ArrayVariable* _values;
  bool _allowDynamicCycles = false;  // Move this?
};

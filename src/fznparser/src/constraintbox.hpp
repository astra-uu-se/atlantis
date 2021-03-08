#pragma once
#include "structure.hpp"
#include "variable.hpp"

class ConstraintBox {
 public:
  ConstraintBox() = default;
  ConstraintBox(std::string name, std::vector<Expression> expressions,
                std::vector<Annotation> annotations);
  void prepare(VariableMap& variables);
  bool hasDefineAnnotation();
  bool hasImplicitAnnotation();
  std::string getAnnotationVariableName();
  void setId(Int id);
  Int _id;
  std::string _name;
  std::vector<Expression> _expressions;
  std::vector<Annotation> _annotations;
};

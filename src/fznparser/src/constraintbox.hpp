#pragma once
#include "structure.hpp"
#include "variable.hpp"

class ConstraintBox {
 public:
  ConstraintBox() = default;
  ConstraintBox(std::string name, std::vector<Expression> expressions,
                std::vector<Annotation> annotations);
  void prepare(std::map<std::string, std::shared_ptr<Variable>>& variables);
  bool hasDefineAnnotation();
  std::string getAnnotationVariableName();
  std::string _name;
  std::vector<Expression> _expressions;
  std::vector<Annotation> _annotations;
};

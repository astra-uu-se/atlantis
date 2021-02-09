#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "structure.hpp"

class ConstraintItem {
 public:
  ConstraintItem();
  ConstraintItem(std::string name, std::vector<Expression> expressions,
                 std::vector<Annotation> annotations);
  std::string _name;
  std::vector<Expression> _expressions;
  std::vector<Annotation> _annotations;
};

class Constraint {
 public:
  Constraint();
  virtual void print() = 0;
  virtual void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) = 0;

  ConstraintItem _constraintItem;
  std::string _name;
};

class IntDiv : public Constraint {
 public:
  IntDiv(ConstraintItem constraintItem);
  void print() override;
  void init(
      std::map<std::string, std::shared_ptr<Variable>> variables) override;
  Variable* _a;
  Variable* _b;
  Variable* _c;
};

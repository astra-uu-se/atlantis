#ifndef __MODEL_HPP_INCLUDED__
#define __MODEL_HPP_INCLUDED__
#include <memory>
#include <string>
#include <vector>

#include "constraint.hpp"

class Model {
 public:
  Model();
  void init();
  void printNode(std::string name);
  void addVariable(std::shared_ptr<Variable> v);
  void addConstraint(ConstraintBox constraintBox);
  void addObjective(std::string objective);
  int definedCount();
  int variableCount();
  std::vector<Constraint*> constraints();
  std::vector<Variable*> variables();
  std::vector<Variable*> domSortVariables();
  void split();
  Variable* objective() { return _objective; }

 private:
  VariableMap _variables;
  ConstraintMap _constraints;
  Variable* _objective = nullptr;
};

#endif

#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "constraint.hpp"
#include "structure.hpp"
#include "variable.hpp"

class Model {
 public:
  Model();
  std::map<std::string, std::shared_ptr<Variable>> _variables;
  std::vector<std::shared_ptr<Constraint>> _constraints;
  void init();
  void addVariable(std::shared_ptr<Variable> v);
  // Variable* getVariable(std::string name);
  // Variable* getParam(std::string name);
  void addConstraint(ConstraintBox constraintBox);
  bool hasCycle();
  void printNode(std::string name);

 private:
  bool hasCycleAux(std::map<Node*, bool> visited, Node* n);
};

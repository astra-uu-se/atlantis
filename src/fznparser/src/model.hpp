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
  bool hasCycle();
  void findStructure();
  void defineAnnotated();
  void tweak();
  void printNode(std::string name);
  void addVariable(std::shared_ptr<Variable> v);
  void addConstraint(ConstraintBox constraintBox);
  int definedCount();

 private:
  bool hasCycleAux(std::set<Node*> visited, Node* n, std::set<Node*>& done);
};

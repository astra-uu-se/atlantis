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
  void init();
  void findStructure();
  void printNode(std::string name);
  void addVariable(std::shared_ptr<Variable> v);
  void addConstraint(ConstraintBox constraintBox);
  int definedCount();
  int variableCount();
  std::vector<Variable*> variables();
  std::vector<Variable*> domSortVariables();

 private:
  void defineAnnotated();
  void defineImplicit();
  void defineFrom(Variable* variable);
  void defineFromObjective();
  void defineUnique();
  void defineRest();
  bool hasCycle();
  bool hasCycleAux(std::set<Node*> visited, Node* n, std::set<Node*>& done);
  void removeCycles();
  void removeCycle(std::set<Node*> visited);
  Variable* getObjective();
  VariableMap _variables;
  std::vector<std::shared_ptr<Constraint>> _constraints;
};

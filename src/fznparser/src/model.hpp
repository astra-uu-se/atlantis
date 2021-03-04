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
  int cyclesRemoved() { return _cyclesRemoved; }
  void setObjective(std::string);
  void split();

  std::vector<std::shared_ptr<Constraint>> _constraints;

 private:
  void defineAnnotated();
  void defineImplicit();
  void defineFrom(Variable* variable);
  void defineFromWithImplicit(Variable* variable);
  void defineFromObjective();
  void defineUnique();
  void defineRest();
  bool hasCycle();
  bool hasCycleAux(std::vector<Node*> visited, Node* n);
  void removeCycles();
  void removeCycle(std::vector<Node*> visited);
  Variable* getObjective();
  VariableMap _variables;
  int _cyclesRemoved = 0;
  std::string _objective;
};

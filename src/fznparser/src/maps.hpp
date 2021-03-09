#pragma once

#include <algorithm>

#include "structure.hpp"
#include "variable.hpp"

// Lägg till soft constraint lista
class Variable;

class VariableMap {
 public:
  VariableMap() = default;
  virtual ~VariableMap() = default;
  Variable* add(std::shared_ptr<Variable> variable);
  Variable* find(std::string name) const;
  bool exists(std::string name);
  std::vector<Variable*> getArray();
  Variable* first() { return *_variableArray.begin(); }

 private:
  std::vector<Variable*> _variableArray;
  std::map<std::string, std::shared_ptr<Variable>> _variables;
};

class ConstraintMap {
 public:
  ConstraintMap() = default;
  virtual ~ConstraintMap() = default;
  void add(std::shared_ptr<Constraint> constraint);
  bool exists(Constraint* constraint);
  void remove(Constraint* constraint);
  std::vector<Constraint*> getVector();
  Int size() { return _constraintArray.size(); }

 private:
  std::vector<Constraint*> _constraintArray;
  std::map<Constraint*, std::shared_ptr<Constraint>> _constraints;
};

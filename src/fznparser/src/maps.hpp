#ifndef __MAPS_HPP_INCLUDED__
#define __MAPS_HPP_INCLUDED__

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

class Constraint;

// Lägg till soft constraint lista
#include "structure.hpp"
#include "variable.hpp"

class VariableMap {
 public:
  VariableMap() = default;
  ~VariableMap() = default;
  Variable* add(std::shared_ptr<Variable> variable);
  ArrayVariable* add(std::shared_ptr<ArrayVariable> variable);
  Variable* find(std::string name) const;
  ArrayVariable* findArray(std::string name) const;
  bool exists(std::string name);
  std::vector<Variable*> variables();
  std::vector<ArrayVariable*> arrays();

 private:
  std::vector<Variable*> _variableVector;
  std::vector<ArrayVariable*> _arrayVector;
  std::map<std::string, std::shared_ptr<Variable>> _variables;
  std::map<std::string, std::shared_ptr<ArrayVariable>> _arrayVariables;
};

class ConstraintMap {
 public:
  ConstraintMap() = default;
  ~ConstraintMap() = default;
  void add(std::shared_ptr<Constraint> constraint);
  bool exists(Constraint* constraint);
  void remove(Constraint* constraint);
  const std::vector<Constraint*>& getVector();
  Int size() { return _constraintArray.size(); }

 private:
  std::vector<Constraint*> _constraintArray;
  std::map<Constraint*, std::shared_ptr<Constraint>> _constraints;
};

#endif

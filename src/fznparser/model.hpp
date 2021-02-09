#pragma once
#include "structure.hpp"
#include "constraint.hpp"

#include <map>
#include <string>
#include <memory>
#include <vector>

// Kolla sort och GlobalCardinality

class Model {
 public:
  Model();
  std::map<std::string, std::shared_ptr<Variable>> _variables;
  std::vector<std::shared_ptr<Constraint>> _constraints;
  void init();
  void addVariable(std::shared_ptr<Variable> v);
  void addConstraint(ConstraintItem ci);
};

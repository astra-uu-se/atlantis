#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "constraint.hpp"

class Model {
 public:
  Model() = default;
  void init();
  void addVariable(std::shared_ptr<Variable> v);
  void addVariable(std::shared_ptr<ArrayVariable> v);
  void addConstraint(ConstraintBox constraintBox);
  void addObjective(std::string objective);
  const std::vector<Constraint*>& constraints();
  VariableMap& varMap();
  const std::vector<Variable*>& domSortVariables();
  const std::vector<Variable*>& potDefSortVariables();
  std::optional<Variable*> objective() { return _objective; }
  void reportDomainChange() { _variables.reportDomainChange(); }
  void reportPotDefChange() { _variables.reportPotDefChange(); }

 private:
  VariableMap _variables;
  ConstraintMap _constraints;
  std::optional<Variable*> _objective;
};

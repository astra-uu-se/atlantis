#ifndef __MODEL_HPP_INCLUDED__
#define __MODEL_HPP_INCLUDED__
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "constraint.hpp"

class Model {
 public:
  Model();
  void init();
  void reset();
  void addVariable(std::shared_ptr<Variable> v);
  void addVariable(std::shared_ptr<ArrayVariable> v);
  void addConstraint(ConstraintBox constraintBox);
  void addObjective(std::string objective);
  std::vector<Constraint*> constraints();
  VariableMap& varMap();
  std::vector<Variable*> domSortVariables();
  std::vector<Variable*> potDefSortVariables();
  std::optional<Variable*> objective() { return _objective; }

 private:
  VariableMap _variables;
  ConstraintMap _constraints;
  std::optional<Variable*> _objective;
};

#endif

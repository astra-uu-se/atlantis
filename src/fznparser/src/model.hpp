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
  void addVariable(std::shared_ptr<Variable> v);
  void addConstraint(ConstraintBox constraintBox);
  void addObjective(std::string objective);
  std::vector<Constraint*> constraints();
  std::vector<Variable*> variables();
  std::vector<Variable*> domSortVariables();
  void split();
  std::optional<Variable*> objective() { return _objective; }

 private:
  VariableMap _variables;
  ConstraintMap _constraints;
  std::optional<Variable*> _objective;
};

#endif

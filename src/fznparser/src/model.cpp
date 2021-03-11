
#include "model.hpp"

#include <unistd.h>

#include <limits>

#include "maps.hpp"

Model::Model() {}
void Model::init() {
  for (auto av : varMap().arrays()) {
    av->init(_variables);
  }
  for (auto constraint : constraints()) {
    constraint->init(_variables);
  }
}
void Model::split() {
  std::string name = constraints()[0]->getName();
  if (constraints()[0]->split(1, _variables, _constraints)) {
    std::cout << "SPLITTING: " << name << std::endl;
  }
}
std::vector<Constraint*> Model::constraints() {
  return _constraints.getVector();
}
VariableMap& Model::varMap() { return _variables; }
std::vector<Variable*> Model::domSortVariables() {
  std::vector<Variable*> sorted =
      varMap().variables();  // Inefficient to sort every time
  std::sort(sorted.begin(), sorted.end(), Variable::compareDomain);
  return sorted;
}

void Model::addVariable(std::shared_ptr<Variable> variable) {
  _variables.add(variable);
}
void Model::addVariable(std::shared_ptr<ArrayVariable> variable) {
  _variables.add(variable);
}
void Model::addObjective(std::string objective) {
  _objective = _variables.find(objective);
}
void Model::addConstraint(ConstraintBox constraintBox) {
  constraintBox.prepare(_variables);
  constraintBox.setId(_constraints.size() + 1);
  if (constraintBox._name == "int_div") {
    _constraints.add(std::make_shared<IntDiv>(constraintBox));
    // } else if (constraintBox._name == "int_plus") {
    // _constraints.add(std::make_shared<IntPlus>(constraintBox));
  } else if (constraintBox._name == "global_cardinality") {
    _constraints.add(std::make_shared<GlobalCardinality>(constraintBox));
  } else if (constraintBox._name == "int_lin_eq") {
    _constraints.add(std::make_shared<IntLinEq>(constraintBox));
    // } else if (constraintBox._name == "int_abs") {
    //   _constraints.add(std::make_shared<IntAbs>(constraintBox));
    // } else if (constraintBox._name == "fzn_all_different_int") {
    //   _constraints.add(std::make_shared<AllDifferent>(constraintBox));
    // } else if (constraintBox._name == "inverse") {
    //   _constraints.add(std::make_shared<Inverse>(constraintBox));
    // } else if (constraintBox._name == "gecode_int_element") {
    //   _constraints.add(std::make_shared<Element>(constraintBox));
    // } else if (constraintBox._name == "gecode_circuit") {
    //   _constraints.add(std::make_shared<Circuit>(constraintBox));
    // } else if (constraintBox._name == "int_le") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "int_lin_le") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "int_lin_ne") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "int_lt") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "int_ne") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "set_in") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "array_bool_xor") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "bool_clause") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "bool_le") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "bool_lin_le") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
    // } else if (constraintBox._name == "bool_lt") {
    //   _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else {
    std::cerr << "Constraint not supported: " + constraintBox._name
              << std::endl;
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  }
}

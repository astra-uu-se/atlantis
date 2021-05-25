
#include "model.hpp"

#include <unistd.h>

#include <limits>

#include "maps.hpp"

void Model::init() {
  for (auto av : varMap().arrays()) {
    av->init(_variables);
  }
  for (auto constraint : constraints()) {
    constraint->init(_variables);
  }
}
const std::vector<Constraint*>& Model::constraints() {
  return _constraints.getVector();
}
VariableMap& Model::varMap() { return _variables; }

// Smaller first
//
const std::vector<Variable*>& Model::potDefSortVariables() {
  return _variables.potDefSort();
}
// Bigger first
const std::vector<Variable*>& Model::domSortVariables() {
  return _variables.domSort();
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
  } else if (constraintBox._name == "int_plus") {
    _constraints.add(std::make_shared<IntPlus>(constraintBox));
  } else if (constraintBox._name == "int_lin_eq") {
    _constraints.add(std::make_shared<IntLinEq>(constraintBox));
  } else if (constraintBox._name == "int_abs") {
    _constraints.add(std::make_shared<IntAbs>(constraintBox));
  } else if (constraintBox._name == "int_max") {
    _constraints.add(std::make_shared<IntMax>(constraintBox));
  } else if (constraintBox._name == "int_min") {
    _constraints.add(std::make_shared<IntMin>(constraintBox));
  } else if (constraintBox._name == "int_mod") {
    _constraints.add(std::make_shared<IntMod>(constraintBox));
  } else if (constraintBox._name == "int_pow") {
    _constraints.add(std::make_shared<IntPow>(constraintBox));
  } else if (constraintBox._name == "int_times") {
    _constraints.add(std::make_shared<IntTimes>(constraintBox));
  } else if (constraintBox._name == "int_eq") {
    _constraints.add(std::make_shared<IntEq>(constraintBox));
  } else if (constraintBox._name == "array_var_int_element") {
    _constraints.add(std::make_shared<VarElement>(constraintBox));
  } else if (constraintBox._name == "array_int_element") {
    _constraints.add(std::make_shared<Element>(constraintBox));
  } else if (constraintBox._name == "array_bool_element") {
    _constraints.add(std::make_shared<BoolElement>(constraintBox));
  } else if (constraintBox._name == "array_var_bool_element") {
    _constraints.add(std::make_shared<VarBoolElement>(constraintBox));
  } else if (constraintBox._name == "array_int_maximum") {
    _constraints.add(std::make_shared<ArrayIntMaximum>(constraintBox));
  } else if (constraintBox._name == "array_int_minimum") {
    _constraints.add(std::make_shared<ArrayIntMinimum>(constraintBox));
  } else if (constraintBox._name == "array_bool_and") {
    _constraints.add(std::make_shared<ArrayBoolAnd>(constraintBox));
  } else if (constraintBox._name == "array_bool_or") {
    _constraints.add(std::make_shared<ArrayBoolOr>(constraintBox));
  } else if (constraintBox._name == "bool2int") {
    _constraints.add(std::make_shared<Bool2Int>(constraintBox));
  } else if (constraintBox._name == "bool_and") {
    _constraints.add(std::make_shared<BoolAnd>(constraintBox));
  } else if (constraintBox._name == "bool_eq") {
    _constraints.add(std::make_shared<BoolEq>(constraintBox));
  } else if (constraintBox._name == "bool_lin_eq") {
    _constraints.add(std::make_shared<BoolLinEq>(constraintBox));
  } else if (constraintBox._name == "bool_not") {
    _constraints.add(std::make_shared<BoolNot>(constraintBox));
  } else if (constraintBox._name == "bool_or") {
    _constraints.add(std::make_shared<BoolOr>(constraintBox));
  } else if (constraintBox._name == "bool_xor") {
    _constraints.add(std::make_shared<BoolXor>(constraintBox));
  } else if (constraintBox._name == "set_in_reif") {
    _constraints.add(std::make_shared<SetInReif>(constraintBox));
  } else if (constraintBox._name == "fzn_global_cardinality") {
    _constraints.add(std::make_shared<GlobalCardinality>(constraintBox));
  } else if (constraintBox._name == "fzn_circuit") {
    _constraints.add(std::make_shared<Circuit>(constraintBox));
  } else if (constraintBox._name == "fzn_subcircuit") {
    _constraints.add(std::make_shared<SubCircuit>(constraintBox));
  } else if (constraintBox._name == "fzn_all_different_int") {
    _constraints.add(std::make_shared<AllDifferent>(constraintBox));
  } else if (constraintBox._name == "int_eq_reif") {
    _constraints.add(std::make_shared<IntEqReif>(constraintBox));
  } else if (constraintBox._name == "int_le_reif") {
    _constraints.add(std::make_shared<IntLeReif>(constraintBox));
  } else if (constraintBox._name == "int_lin_eq_reif") {
    _constraints.add(std::make_shared<IntLinEqReif>(constraintBox));
  } else if (constraintBox._name == "int_lin_le_reif") {
    _constraints.add(std::make_shared<IntLinLeReif>(constraintBox));
  } else if (constraintBox._name == "int_lin_ne_reif") {
    _constraints.add(std::make_shared<IntLinNeReif>(constraintBox));
  } else if (constraintBox._name == "int_lt_reif") {
    _constraints.add(std::make_shared<IntLtReif>(constraintBox));
  } else if (constraintBox._name == "int_ne_reif") {
    _constraints.add(std::make_shared<IntNeReif>(constraintBox));
  } else if (constraintBox._name == "bool_eq_reif") {
    _constraints.add(std::make_shared<BoolEqReif>(constraintBox));
  } else if (constraintBox._name == "bool_le_reif") {
    _constraints.add(std::make_shared<BoolLeReif>(constraintBox));
  } else if (constraintBox._name == "bool_lt_reif") {
    _constraints.add(std::make_shared<BoolLtReif>(constraintBox));
  } else if (constraintBox._name == "int_le") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "int_lin_le") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "int_lin_ne") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "int_lt") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "int_ne") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "set_in") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "array_bool_xor") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "bool_clause") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "bool_le") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "bool_lin_le") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else if (constraintBox._name == "bool_lt") {
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  } else {
    std::cerr << "Constraint not supported: " + constraintBox._name
              << std::endl;
  }
}

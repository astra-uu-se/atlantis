#include "model.hpp"


Model::Model(){};
void Model::init() {
  for (auto constraint : _constraints) {
    constraint->init(_variables);
  }
}
void Model::addVariable(std::shared_ptr<Variable> variable) {
  _variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
      variable->_name, variable));
}
void Model::addConstraint(ConstraintItem constraintItem) {
  if (constraintItem._name == "int_div") {
    _constraints.push_back(std::make_shared<IntDiv>(constraintItem));
  }
}

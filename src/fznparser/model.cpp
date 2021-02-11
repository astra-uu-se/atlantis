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
  } else if (constraintItem._name == "int_max") {
    _constraints.push_back(std::make_shared<IntMax>(constraintItem));
  } else if (constraintItem._name == "int_plus") {
    _constraints.push_back(std::make_shared<IntPlus>(constraintItem));
  }
}
bool Model::hasCycle() {
  for (auto n_pair : _variables) {
    auto n = n_pair.second.get();
    if (!n->isDefined() && !n->getNext().empty()) {
      std::map<Node*, bool> visited;
      if (hasCycleAux(visited, n)) return true;
    }
  }
  return false;
}
bool Model::hasCycleAux(std::map<Node*, bool> visited, Node* n) {
  if (visited.count(n)) return true;
  visited.insert(std::pair<Node*, bool>(n, true));
  for (auto m : n->getNext()) {
    if (hasCycleAux(visited, m)) return true;
  }
  return false;
}

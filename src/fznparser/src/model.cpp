#include "model.hpp"

Model::Model(){};
void Model::init() {
  for (auto item : _variables) {
    item.second.get()->init(_variables);
  }
  for (auto constraint : _constraints) {
    constraint->init(_variables);
  }
}
void Model::addVariable(std::shared_ptr<Variable> item) {
  _variables.insert(std::pair<std::string, std::shared_ptr<Variable>>(
      item->getName(), item));
}
void Model::addConstraint(ConstraintBox constraintBox) {
  if (constraintBox._name == "int_div") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<IntDiv>(constraintBox));
  } else if (constraintBox._name == "int_plus") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<IntPlus>(constraintBox));
  } else if (constraintBox._name == "global_cardinality") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<GlobalCardinality>(constraintBox));
  } else if (constraintBox._name == "int_lin_eq") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<IntLinEq>(constraintBox));
  }
}
bool Model::hasCycle() {
  for (auto n_pair : _variables) {
    std::cout << "Starting...\n";
    auto n = n_pair.second.get();
    std::map<Node*, bool> visited;
    if (hasCycleAux(visited, n)) return true;
  }
  return false;
}
bool Model::hasCycleAux(std::map<Node*, bool> visited, Node* n) {
  std::cout << "Node: " << n->getLabel() << std::endl;
  if (visited.count(n)) return true;
  visited.insert(std::pair<Node*, bool>(n, true));
  for (auto m : n->getNext()) {
    if (hasCycleAux(visited, m)) return true;
  }
  return false;
}

void Model::printNode(std::string name) {
  assert(_variables.find(name) != _variables.end());
  Node *node = _variables.find(name)->second.get();
  std::cout << node->getLabel() << std::endl;
}

void Model::tweak() {
  for (auto c : _constraints) {
    c->tweak();
  }
}

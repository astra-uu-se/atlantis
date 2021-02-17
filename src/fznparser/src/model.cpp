#include "model.hpp"

Model::Model(){};
void Model::init() {
  for (auto item : _items) {
    item.second.get()->init(_items);
  }
  for (auto constraint : _constraints) {
    constraint->init(_items);
  }
}
void Model::addItem(std::shared_ptr<Item> item) {
  _items.insert(std::pair<std::string, std::shared_ptr<Item>>(
      item->getName(), item));
}
// Item* Model::getItem(std::string name) {
//   assert(_items.find(name) != _items.end());
//   return _items.find(name)->second.get();
// }

// Item* Model::getParam(std::string name) {
//   if (_items.find(name) != _items.end()) {
//     return _items.find(name)->second.get();
//   } else {
//     addItem(std::make_shared<Parameter>(name));
//     return _items.find(name)->second.get();
//   }
// }

void Model::addConstraint(ConstraintBox constraintBox) {
  if (constraintBox._name == "int_div") {
    constraintBox.prepare(_items);
    _constraints.push_back(std::make_shared<IntDiv>(constraintBox));

  // } else if (constraintBox._name == "int_max") {
  //   _constraints.push_back(std::make_shared<IntMax>(constraintBox));
  // } else if (constraintBox._name == "int_plus") {
  //   _constraints.push_back(std::make_shared<IntPlus>(constraintBox));
  // } else if (constraintBox._name == "global_cardinality") {
  } else if (constraintBox._name == "global_cardinality") {
    constraintBox.prepare(_items);
    _constraints.push_back(std::make_shared<GlobalCardinality>(constraintBox));
  } else if (constraintBox._name == "int_lin_eq") {
    constraintBox.prepare(_items);
    _constraints.push_back(std::make_shared<IntLinEq>(constraintBox));
  }
  // }
}
bool Model::hasCycle() {
  for (auto n_pair : _items) {
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
  assert(_items.find(name) != _items.end());
  Node *node = _items.find(name)->second.get();
  std::cout << node->getLabel() << std::endl;
}

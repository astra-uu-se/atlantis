#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "constraint.hpp"
#include "structure.hpp"

class Model {
 public:
  Model();
  std::map<std::string, std::shared_ptr<Item>> _items;
  std::vector<std::shared_ptr<Constraint>> _constraints;
  void init();
  void addItem(std::shared_ptr<Item> v);
  // Item* getItem(std::string name);
  // Item* getParam(std::string name);
  void addConstraint(ConstraintBox constraintBox);
  bool hasCycle();
  void printNode(std::string name);

 private:
  bool hasCycleAux(std::map<Node*, bool> visited, Node* n);
};

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

void Model::findStructure() {
  defineImplicit();
  defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  removeCycles();
}

void Model::defineAnnotated() {
  for (auto c : _constraints) {
    if (c->canDefineByAnnotation()) {  // Also checks that target is not defined
      c->makeOneWayByAnnotation();
    }
  }
}
void Model::defineImplicit() {
  for (auto c : _constraints) {
    if (c->canBeImplicit()) {  // Also checks that target is not defined
      c->makeImplicit();
    }
  }
}

void Model::defineFrom(Variable* variable) {
  std::cout << "Defining from..." << variable->getLabel() << std::endl;

  if (variable->isDefinable()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->definesNone()) {
        constraint->makeOneWay(variable);
        for (auto v : constraint->variables()) {
          if (v != variable) {
            defineFrom(v);
          }
        }
        break;
      }
    }
  }
}
void Model::defineFromObjective() {
  if (true) {
    Variable* objective = getObjective();
    defineFrom(objective);
  }
}
Variable* Model::getObjective() {
  return _variables.find("X_INTRODUCED_0_")->second.get();
}

void Model::defineUnique() {
  for (auto item : _variables) {
    auto variable = item.second.get();
    if (variable->isDefinable()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->uniqueTarget() && constraint->definesNone()) {
          constraint->makeOneWay(variable);
          break;
        }
      }
    }
  }
}
void Model::defineRest() {
  for (auto item : _variables) {
    auto variable = item.second.get();
    if (variable->isDefinable()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->definesNone()) {
          constraint->makeOneWay(variable);
          break;
        }
      }
    }
  }
}

int Model::variableCount() {
  int n = 0;
  for (auto v : _variables) {
    n += v.second->count();
  }
  return n;
}
int Model::definedCount() {
  int n = 0;
  for (auto v : _variables) {
    if (v.second->isDefined()) {
      n += v.second->count();
    }
  }
  return n;
}

void Model::addVariable(std::shared_ptr<Variable> item) {
  _variables.insert(
      std::pair<std::string, std::shared_ptr<Variable>>(item->getName(), item));
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
  } else if (constraintBox._name == "int_abs") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<IntAbs>(constraintBox));
  } else if (constraintBox._name == "fzn_all_different_int") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<AllDifferent>(constraintBox));
  } else if (constraintBox._name == "inverse") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<Inverse>(constraintBox));
  }
}

bool Model::hasCycle() {
  std::set<Node*> done;
  for (auto n_pair : _variables) {
    std::cout << "Starting...\n";
    auto n = n_pair.second.get();
    std::set<Node*> visited;
    if (hasCycleAux(visited, n, done)) return true;
  }
  return false;
}
bool Model::hasCycleAux(std::set<Node*> visited, Node* n,
                        std::set<Node*>& done) {
  if (done.count(n)) return false;
  std::cout << "Node: " << n->getLabel() << std::endl;
  if (visited.count(n)) {
    removeCycle(visited);
    return true;
  }
  visited.insert(n);
  for (auto m : n->getNext()) {
    if (hasCycleAux(visited, m, done)) return true;
  }
  done.insert(visited.begin(), visited.end());
  return false;
}
void Model::removeCycle(std::set<Node*> visited) {
  for (auto node : visited) {
    if (node->breakCycle()) {
      return;
    }
  }
}

void Model::removeCycles() {
  std::cout << "Looking for Cycles..." << std::endl;
  while (hasCycle()) {
    std::cout << "Cycle found...Removing" << std::endl;
  }
  std::cout << "Done! No cycles." << std::endl;
}

void Model::printNode(std::string name) {
  assert(_variables.find(name) != _variables.end());
  Node* node = _variables.find(name)->second.get();
  std::cout << node->getLabel() << std::endl;
}

#include "model.hpp"

Model::Model(){};
void Model::init() {
  for (auto item : _variables) {
    item.second.get()->init(_variables);
  }
  for (auto constraint : _constraints) {
    constraint->init(_variables);
    // constraint->makeOneWay();
  }
}

void Model::findStructure() {
  defineAnnotated();
  defineFromObjective();
  // defineWithFunctional();
  // defineWithAny();
  // removeCycles();
}

/* All constraints annotated with defines_var in the FlatZinc model are made
one-way constraints. */
void Model::defineAnnotated() {
  for (auto c : _constraints) {
    if (c->hasDefineAnnotation()) {
      c->defineByAnnotation();
    }
  }
}

void Model::defineFrom(Variable* variable) {
  for (auto constraint : variable->_potentialDefiners) {
    if (constraint->_defines.empty()) {
      constraint->forceOneWay(variable);
      for (auto v : constraint->_variables) {
        if (v != variable) {
          defineFrom(v);
        }
      }
      break;
    }
  }
}
void Model::defineFromObjective() {
  Variable* objective = getObjective();
  return defineFrom(objective);
}
Variable* Model::getObjective() { return _variables.find("a")->second.get(); }

int Model::definedCount() {
  int n = 0;
  for (auto v : _variables) {
    if (v.second->isDefined()) {
      n++;
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
    // } else if (constraintBox._name == "int_lin_eq") {
    // constraintBox.prepare(_variables);
    // _constraints.push_back(std::make_shared<IntLinEq>(constraintBox));
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
  if (visited.count(n)) return true;
  visited.insert(n);
  for (auto m : n->getNext()) {
    if (hasCycleAux(visited, m, done)) return true;
  }
  done.insert(visited.begin(), visited.end());
  return false;
}

void Model::printNode(std::string name) {
  assert(_variables.find(name) != _variables.end());
  Node* node = _variables.find(name)->second.get();
  std::cout << node->getLabel() << std::endl;
}

void Model::tweak() {
  for (auto c : _constraints) {
    c->tweak();
  }
}

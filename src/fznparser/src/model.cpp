#include "model.hpp"

#include <limits>

Model::Model() {}
void Model::init() {
  for (auto variable : variables()) {
    variable->init(_variables);
  }
  for (auto constraint : _constraints) {
    constraint->init(_variables);
  }
}

std::vector<Variable*> Model::variables() { return _variables.getArray(); }
std::vector<Variable*> Model::domSortVariables() {
  std::vector<Variable*> sorted =
      variables();  // Inefficient to sort every time
  std::sort(sorted.begin(), sorted.end(), Variable::compareDomain);
  return sorted;
}

void Model::findStructure() {
  defineImplicit();
  defineRest();
  defineAnnotated();
  defineFromObjective();
  defineUnique();
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
  // std::cout << "Defining from..." << variable->getLabel() << std::endl;
  if (variable->isDefinable()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->definesNone()) {
        constraint->makeOneWay(variable);
        // Sort by domain size
        std::vector<Variable*> next = constraint->variables();
        std::sort(next.begin(), next.end(), Variable::compareDomain);
        for (auto v : next) {
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
Variable* Model::getObjective() { return _variables.find("X_INTRODUCED_0_"); }

void Model::defineUnique() {
  for (auto variable : domSortVariables()) {
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
  for (auto variable : domSortVariables()) {
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
  for (auto variable : variables()) {
    n += variable->count();
  }
  return n;
}
int Model::definedCount() {
  int n = 0;
  for (auto variable : variables()) {
    if (variable->isDefined()) {
      n += variable->count();
    }
  }
  return n;
}

void Model::addVariable(std::shared_ptr<Variable> variable) {
  _variables.add(variable);
}

bool Model::hasCycle() {
  std::set<Node*> done;
  for (auto node : variables()) {
    std::cout << "Starting...\n";
    std::set<Node*> visited;
    if (hasCycleAux(visited, node, done)) return true;
  }
  return false;
}
bool Model::hasCycleAux(std::set<Node*> visited, Node* node,
                        std::set<Node*>& done) {
  if (done.count(node)) return false;
  std::cout << "Node: " << node->getLabel() << std::endl;
  if (visited.count(node)) {
    removeCycle(visited);
    return true;
  }
  visited.insert(node);
  for (auto next : node->getNext()) {
    if (hasCycleAux(visited, next, done)) return true;
  }
  done.insert(visited.begin(), visited.end());
  return false;
}
void Model::removeCycle(std::set<Node*> visited) {
  unsigned int smallestDomain = -1;  // Förmodligen inte så bra
  Node* nodeToRemove = nullptr;
  for (auto node : visited) {
    if (auto v = dynamic_cast<Variable*>(node)) {
      if (v->domainSize() < smallestDomain) {  // Check ties
        nodeToRemove = v->definedBy();
        smallestDomain = v->domainSize();
      }
    }
  }
  assert(nodeToRemove->breakCycle());
}

void Model::removeCycles() {
  std::cout << "Looking for Cycles..." << std::endl;
  while (hasCycle()) {
    std::cout << "Cycle found...Removing" << std::endl;
  }
  std::cout << "Done! No cycles." << std::endl;
}

void Model::printNode(std::string name) {
  Node* node = _variables.find(name);
  std::cout << node->getLabel() << std::endl;
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
  } else if (constraintBox._name == "gecode_int_element") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<Element>(constraintBox));
  } else if (constraintBox._name == "gecode_circuit") {
    constraintBox.prepare(_variables);
    _constraints.push_back(std::make_shared<Circuit>(constraintBox));
  }
}

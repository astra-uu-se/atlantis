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
void Model::split() {
  // for (int i = 0; i < _constraints.size(); i++) {
  if (_constraints[0]->split(2, _variables, _constraints)) {
    std::cout << "HELLO" << std::endl;

    _constraints.erase(_constraints.begin() + 0);
  }
  // }
}
void Model::setObjective(std::string objective) { _objective = objective; }

std::vector<Variable*> Model::variables() { return _variables.getArray(); }
std::vector<Variable*> Model::domSortVariables() {
  std::vector<Variable*> sorted =
      variables();  // Inefficient to sort every time
  std::sort(sorted.begin(), sorted.end(), Variable::compareDomain);
  return sorted;
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

void Model::defineFromWithImplicit(Variable* variable) {
  if (variable->isDefinable()) {
    std::cout << "is definable" << std::endl;

    for (auto constraint : variable->potentialDefiners()) {
      std::cout << constraint->canBeOneWay(variable) << std::endl;
      std::cout << constraint->getLabel() << std::endl;

      if (constraint->canBeImplicit()) {
        std::cout << "can be implicit" << std::endl;
        constraint->makeImplicit();
        return;

      } else if (constraint->canBeOneWay(variable) &&
                 constraint->definesNone()) {
        std::cout << "can be one way" << std::endl;
        constraint->makeOneWay(variable);
        for (auto v : constraint->variablesSorted()) {
          if (v != variable) {
            defineFrom(v);
          }
        }
        break;
      }
    }
  }
}
void Model::defineFrom(Variable* variable) {
  if (variable->isDefinable()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->canBeOneWay(variable) && constraint->definesNone()) {
        constraint->makeOneWay(variable);
        for (auto v : constraint->variablesSorted()) {
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
    defineFromWithImplicit(objective);
  }
}
Variable* Model::getObjective() { return _variables.find(_objective); }

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
  for (auto node : variables()) {
    std::cout << "Starting...\n";
    std::vector<Node*> visited;
    if (hasCycleAux(visited, node)) return true;
  }
  return false;
}
bool Model::hasCycleAux(std::vector<Node*> visited, Node* node) {
  std::cout << "Node: " << node->getLabel() << std::endl;
  if (std::count(visited.begin(), visited.end(), node)) {
    std::cout << "Cycle found...Removing" << std::endl;
    visited.erase(visited.begin(),
                  std::find(visited.begin(), visited.end(), node));
    removeCycle(visited);
    return true;
  }
  visited.push_back(node);
  for (auto next : node->getNext()) {
    if (hasCycleAux(visited, next)) return true;
  }
  return false;
}
void Model::removeCycle(std::vector<Node*> visited) {
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
  std::cout << "Node: " << nodeToRemove->getLabel() << " removed." << std::endl;
  _cyclesRemoved += 1;
}

void Model::removeCycles() {
  std::cout << "Looking for Cycles..." << std::endl;
  while (hasCycle()) {
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

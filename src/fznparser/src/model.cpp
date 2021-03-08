#include "constraint.hpp"
#define TRACK false
#include <unistd.h>

#include <limits>

#include "maps.hpp"
#include "model.hpp"

Model::Model() {}
void Model::init() {
  for (auto variable : variables()) {
    variable->init(_variables);
  }
  for (auto constraint : constraints()) {
    constraint->init(_variables);
  }
}
void Model::split() {
  std::string name = constraints()[0]->getLabel();
  if (constraints()[0]->split(1, _variables, _constraints)) {
    std::cout << "SPLITTING: " << name << std::endl;
  }
}
std::vector<Constraint*> Model::constraints() {
  return _constraints.getArray();
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
  defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  removeCycles();
}
void Model::defineAnnotated() {
  for (auto c : constraints()) {
    if (c->canDefineByAnnotation()) {  // Also checks that target is not defined
      c->makeOneWayByAnnotation();
    }
  }
}
void Model::defineImplicit() {
  for (auto c : constraints()) {
    if (c->canBeImplicit() &&
        c->shouldBeImplicit()) {  // Also checks that target is not defined
      c->makeImplicit();
    }
  }
}
void Model::defineFromWithImplicit(Variable* variable) {
  if (variable->isDefinable()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->canBeOneWay(variable) && constraint->definesNone()) {
        constraint->makeOneWay(variable);
        for (auto v : constraint->variablesSorted()) {
          if (v != variable) {
            defineFromWithImplicit(v);
          }
        }
        break;
      } else if (constraint->canBeImplicit()) {
        constraint->makeImplicit();
        return;
      }
    }
  }
}
void Model::defineFromWithImplicit2(Variable* variable) {
  if (variable->isDefinable()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->canBeImplicit()) {
        constraint->makeImplicit();
        return;
      } else if (constraint->canBeOneWay(variable) &&
                 constraint->definesNone()) {
        constraint->makeOneWay(variable);
        for (auto v : constraint->variablesSorted()) {
          if (v != variable) {
            defineFromWithImplicit2(v);
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
  if (_objective) {
    defineFromWithImplicit(_objective);
  } else {
    std::cerr << "No objective exists\n";
  }
}
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
void Model::addObjective(std::string objective) {
  _objective = _variables.find(objective);
}
void Model::removeCycle(std::vector<Node*> visited) {
  Int smallestDomain = std::numeric_limits<Int>::max();
  Constraint* nodeToRemove = nullptr;
  for (auto node : visited) {
    if (auto v = dynamic_cast<Variable*>(node)) {
      if (v->domainSize() <= smallestDomain) {
        if (v->domainSize() < smallestDomain) {
          nodeToRemove = v->definedBy();
          smallestDomain = v->domainSize();
        } else {
          if (v->definedBy()->defInVarCount() > nodeToRemove->defInVarCount()) {
            nodeToRemove = v->definedBy();
            smallestDomain = v->domainSize();
          }
        }
      }
    }
  }
  assert(nodeToRemove->breakCycle());
  std::cout << (TRACK ? "Node: " + nodeToRemove->getLabel() + " removed." + "\n"
                      : "");
  _cyclesRemoved += 1;
}
void Model::removeCycles() {
  std::cout << (TRACK ? "Looking for Cycles...\n" : "");
  std::vector<Node*> cycle;
  while (true) {
    cycle = hasCycle();
    if (cycle.size() > 0) {
      removeCycle(cycle);
    } else {
      break;
    }
  }
  std::cout << (TRACK ? "Done! No cycles.\n" : "");
}

std::vector<Node*> Model::hasCycle() {
  std::set<Node*> visited;
  std::vector<Node*> stack;
  for (auto node : variables()) {
    if (hasCycleAux(visited, stack, node)) {
      return stack;
    }
  }
  assert(stack.size() == 0);
  return stack;
}

bool Model::hasCycleAux(std::set<Node*>& visited, std::vector<Node*>& stack,
                        Node* node) {
  stack.push_back(node);
  if (!visited.count(node)) {
    visited.insert(node);
    for (auto next : node->getNext()) {
      if ((visited.count(next) == 0) && hasCycleAux(visited, stack, next)) {
        return true;
      } else if (std::count(stack.begin(), stack.end(), next)) {
        stack.erase(stack.begin(), std::find(stack.begin(), stack.end(), next));
        return true;
      }
    }
  }
  stack.pop_back();
  return false;
}

void Model::printNode(std::string name) {
  Node* node = _variables.find(name);
  std::cout << node->getLabel() << std::endl;
}
void Model::addConstraint(ConstraintBox constraintBox) {
  constraintBox.prepare(_variables);
  constraintBox.setId(_constraints.size() + 1);
  if (constraintBox._name == "int_div") {
    _constraints.add(std::make_shared<IntDiv>(constraintBox));
  } else if (constraintBox._name == "int_plus") {
    _constraints.add(std::make_shared<IntPlus>(constraintBox));
  } else if (constraintBox._name == "global_cardinality") {
    _constraints.add(std::make_shared<GlobalCardinality>(constraintBox));
  } else if (constraintBox._name == "int_lin_eq") {
    _constraints.add(std::make_shared<IntLinEq>(constraintBox));
  } else if (constraintBox._name == "int_abs") {
    _constraints.add(std::make_shared<IntAbs>(constraintBox));
  } else if (constraintBox._name == "fzn_all_different_int") {
    _constraints.add(std::make_shared<AllDifferent>(constraintBox));
  } else if (constraintBox._name == "inverse") {
    _constraints.add(std::make_shared<Inverse>(constraintBox));
  } else if (constraintBox._name == "gecode_int_element") {
    _constraints.add(std::make_shared<Element>(constraintBox));
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
    // std::cerr << "Constraint not supported: " + constraintBox._name
    //           << std::endl;
    _constraints.add(std::make_shared<NonFunctionalConstraint>(constraintBox));
  }
}

#include "structure_scheme_1.hpp"
#define TRACK true

void StructureScheme1::findStructure() {
  defineImplicit();
  defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  defineByImplicit();
  removeCycles();
}
void StructureScheme1::defineAnnotated() {
  for (auto c : _m->constraints()) {
    if (c->canDefineByAnnotation()) {  // Also checks that target is not defined
      c->makeOneWayByAnnotation();
    }
  }
}
void StructureScheme1::defineImplicit() {
  for (auto c : _m->constraints()) {
    if (c->canBeImplicit() &&  // Also checks that target is not defined
        c->shouldBeImplicit()) {
      c->makeImplicit();
    }
  }
}
// Kolla domänens utveckling först
void StructureScheme1::defineFromWithImplicit(Variable* variable) {
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
void StructureScheme1::defineFromWithImplicit2(Variable* variable) {
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
void StructureScheme1::defineFrom(Variable* variable) {
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
void StructureScheme1::defineFromObjective() {
  if (_m->objective()) {
    defineFromWithImplicit(_m->objective());
  } else {
    std::cerr << "No objective exists\n";
  }
}
void StructureScheme1::defineUnique() {
  for (auto variable : _m->domSortVariables()) {
    if (variable->isDefinable()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->canBeOneWay(variable) && constraint->uniqueTarget() &&
            constraint->definesNone()) {
          constraint->makeOneWay(variable);
          break;
        }
      }
    }
  }
}
void StructureScheme1::defineRest() {
  for (auto variable : _m->domSortVariables()) {
    if (variable->isDefinable()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->canBeOneWay(variable) && constraint->definesNone()) {
          constraint->makeOneWay(variable);
          break;
        }
      }
    }
  }
}
void StructureScheme1::defineByImplicit() {
  for (auto variable : _m->domSortVariables()) {
    if (variable->isDefinable()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->canBeImplicit()) {
          constraint->makeImplicit();
          break;
        }
      }
    }
  }
}

void StructureScheme1::removeCycle(std::vector<Node*> visited) {
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
void StructureScheme1::removeCycles() {
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

std::vector<Node*> StructureScheme1::hasCycle() {
  std::set<Node*> visited;
  std::vector<Node*> stack;
  for (auto node : _m->variables()) {
    if (hasCycleAux(visited, stack, node)) {
      return stack;
    }
  }
  assert(stack.size() == 0);
  return stack;
}

bool StructureScheme1::hasCycleAux(std::set<Node*>& visited,
                                   std::vector<Node*>& stack, Node* node) {
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

#include "structure_scheme_1.hpp"
#define TRACK false

void StructureScheme1::scheme1() {
  // defineByImplicit();
  // defineImplicit();
  // defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  removeCycles(false);
  updateDomains();
}

void StructureScheme1::scheme2() {
  defineFromObjective();
  defineLeastUsed();
  removeCycles(false);
  updateDomains();
}
void StructureScheme1::scheme3() {
  defineFromObjective();
  defineUnique();
  defineRest();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineFromObjective();
    defineUnique();
    defineRest();
  }
}
void StructureScheme1::scheme4() {
  defineFromObjective();
  defineLeastUsed();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineFromObjective();
    defineLeastUsed();
  }
}
void StructureScheme1::scheme5() {
  defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineFromObjective();
    defineUnique();
    defineRest();
  }
}
void StructureScheme1::scheme6() {
  defineAnnotated();
  defineFromObjective();
  defineLeastUsed();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineFromObjective();
    defineLeastUsed();
  }
}
void StructureScheme1::clear() {
  for (auto constraint : _m->constraints()) {
    constraint->makeSoft();
  }
  for (auto var : _m->varMap().variables()) {
    assert(!var->isDefined());
  }
  // for (auto var : _m->varMap().variables()) {
  //   var->removeDefinition();
  // }
}
void StructureScheme1::updateDomains() {
  for (auto constraint : _m->constraints()) {  // _m->conMap.functional()
    if (constraint->isInvariant()) {
      std::set<Constraint*> visited;
      constraint->refreshAndPropagate(visited);
    }
  }
}
void StructureScheme1::defineAnnotated() {
  for (auto c : _m->constraints()) {
    if (c->annotationTarget().has_value() &&
        !c->annotationTarget().value()->isDefined()) {
      c->define(c->annotationTarget().value());
    }
  }
}
void StructureScheme1::defineImplicit() {
  for (auto c : _m->constraints()) {
    if (c->canBeImplicit() && c->shouldBeImplicit()) {
      c->makeImplicit();
    }
  }
}
// Kolla domänens utveckling först
void StructureScheme1::defineFromWithImplicit(Variable* variable) {
  if (variable->isDefinable() && !variable->isDefined()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->canDefine(variable) && constraint->definesNone()) {
        constraint->define(variable);
        for (auto v : constraint->variablesSorted()) {
          if (v != variable) {
            defineFromWithImplicit(v);
          }
        }
        break;
      } else if (constraint->canBeImplicit() &&
                 constraint->allVariablesFree()) {
        constraint->makeImplicit();
        return;
      }
    }
  }
}
void StructureScheme1::defineFrom(Variable* variable) {
  if (variable->isDefinable()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->canDefine(variable) && constraint->definesNone()) {
        constraint->define(variable);
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
  if (_m->objective().has_value()) {
    defineFromWithImplicit(_m->objective().value());
  } else {
    // std::cerr << "No objective exists\n";
  }
}
void StructureScheme1::defineUnique() {
  for (auto variable : _m->domSortVariables()) {
    if (variable->isDefinable() && !variable->isDefined()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->canDefine(variable) && constraint->uniqueTarget() &&
            constraint->definesNone()) {
          constraint->define(variable);
          break;
        }
      }
    }
  }
}
void StructureScheme1::defineRest() {
  for (auto variable : _m->domSortVariables()) {
    if (variable->isDefinable() && !variable->isDefined()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->canDefine(variable) && constraint->definesNone()) {
          constraint->define(variable);
          break;
        }
      }
    }
  }
}
void StructureScheme1::defineByImplicit() {
  for (auto variable : _m->domSortVariables()) {
    if (variable->isDefinable() && !variable->isDefined()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->canBeImplicit()) {
          constraint->makeImplicit();
          std::cout << constraint->getName() << std::endl;
          break;
        }
      }
    }
  }
}

void StructureScheme1::removeCycle(std::vector<Node*> visited, bool ban) {
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
  if (ban) {
    assert(nodeToRemove->breakCycleWithBan());
  } else {
    assert(nodeToRemove->breakCycle());
  }
  std::cout << (TRACK ? "Node: " + nodeToRemove->getName() + " removed." + "\n"
                      : "");
  _cyclesRemoved += 1;
}
void StructureScheme1::removeCycles(bool ban) {
  std::cout << (TRACK ? "Looking for Cycles...\n" : "");
  std::vector<Node*> cycle;
  while (true) {
    cycle = hasCycle();
    if (cycle.size() > 0) {
      removeCycle(cycle, ban);
    } else {
      break;
    }
  }
  std::cout << (TRACK ? "Done! No cycles.\n" : "");
}

std::vector<Node*> StructureScheme1::hasCycle() {
  std::set<Node*> visited;
  std::vector<Node*> stack;
  for (auto node : _m->varMap().variables()) {
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

void StructureScheme1::defineLeastUsed() {
  for (auto var : _m->potDefSortVariables()) {
    if (!var->isDefined()) {
      for (auto con : var->potentialDefiners()) {
        if (con->definesNone()) {  // Add better prio
          con->define(var);
          break;
        }
      }
    }
  }
}

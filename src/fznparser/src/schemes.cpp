#include "schemes.hpp"
#define TRACK false

void Schemes::scheme1() {
  reset();
  // defineByImplicit();
  // defineImplicit();
  // defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  removeCycles(false);
  updateDomains();
}
void Schemes::scheme2() {
  reset();
  defineFromObjective();
  defineLeastUsed();
  removeCycles(false);
  updateDomains();
}
void Schemes::scheme3() {
  reset();
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
void Schemes::scheme4() {
  reset();
  defineFromObjective();
  defineLeastUsed();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineFromObjective();
    defineLeastUsed();
  }
}
void Schemes::scheme5() {
  reset();
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
void Schemes::scheme6() {
  reset();
  defineLeastUsed();
  defineAnnotated();
  defineFromObjective();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineFromObjective();
    defineLeastUsed();
  }
}
void Schemes::reset() {
  for (auto constraint : _m->constraints()) {
    constraint->makeSoft();
    constraint->init(_m->varMap());
  }
  for (auto var : _m->varMap().variables()) {
    assert(!var->hasImposedDomain());
    assert(!var->isDefined());
  }
}
void Schemes::updateDomains() {
  for (auto constraint : _m->constraints()) {  // _m->conMap.functional()
    if (constraint->isInvariant()) {
      std::set<Constraint*> visited;
      constraint->refreshAndPropagate(visited);
    }
  }
}
void Schemes::defineAnnotated() {
  for (auto c : _m->constraints()) {
    if (c->annotationTarget().has_value() &&
        !c->annotationTarget().value()->isDefined() &&
        (c->annotationTarget().value()->hasPotentialDefiner(c))) {
      c->define(c->annotationTarget().value());
    }
  }
}
void Schemes::defineImplicit() {
  for (auto c : _m->constraints()) {
    if (c->canBeImplicit() && c->shouldBeImplicit()) {
      c->makeImplicit();
    }
  }
}
void Schemes::defineFromWithImplicit(Variable* variable) {
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
void Schemes::defineFrom(Variable* variable) {
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
void Schemes::defineFromObjective() {
  if (_m->objective().has_value()) {
    defineFromWithImplicit(_m->objective().value());
  } else {
    // std::cerr << "No objective exists\n";
  }
}
void Schemes::defineUnique() {
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
void Schemes::defineRest() {
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
void Schemes::defineByImplicit() {
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
void Schemes::removeCycle(std::vector<Node*> visited, bool ban) {
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
void Schemes::removeCycles(bool ban) {
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
std::vector<Node*> Schemes::hasCycle() {
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
bool Schemes::hasCycleAux(std::set<Node*>& visited, std::vector<Node*>& stack,
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

void Schemes::defineLeastUsed() {
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

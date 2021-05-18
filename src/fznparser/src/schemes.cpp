#include "schemes.hpp"

#include <algorithm>
#define TRACK false

std::string Schemes::name() { return _name; }

void Schemes::scheme1() {
  _name = "OscNA\t";
  reset();
  defineFromObjective();
  defineUnique();
  defineRest();
  defineImplicit();
  removeCycles(false);
  updateDomains();
}
void Schemes::scheme2() {
  _name = "LUNA\t";
  reset();
  defineLeastUsed();
  removeCycles(false);
  updateDomains();
}
void Schemes::scheme3() {
  _name = "OscRepNA";
  reset();
  defineFromObjective();
  defineUnique();
  defineRest();
  defineImplicit();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineFromObjective();
    defineUnique();
    defineRest();
    defineImplicit();
  }
}
void Schemes::scheme4() {
  _name = "LURepNA\t";
  reset();
  defineLeastUsed();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineLeastUsed();
  }
}
void Schemes::scheme5() {
  _name = "Osc\t";
  reset();
  defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  defineImplicit();
  removeCycles(false);
  updateDomains();
}
void Schemes::scheme6() {
  _name = "LU\t";
  reset();
  defineAnnotated();
  defineLeastUsed();
  removeCycles(false);
  updateDomains();
}
void Schemes::scheme7() {
  _name = "OscRep\t";
  reset();
  defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  defineImplicit();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineAnnotated();
    defineFromObjective();
    defineUnique();
    defineRest();
    defineImplicit();
  }
}
void Schemes::scheme8() {
  _name = "LURep\t";
  reset();
  defineAnnotated();
  defineLeastUsed();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineAnnotated();
    defineLeastUsed();
  }
}
void Schemes::scheme9() {
  _name = "ImpAnnOsc";
  reset();
  defineImplicit();
  defineAnnotated();
  defineFromObjective();
  defineUnique();
  defineRest();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineImplicit();
    defineAnnotated();
    defineFromObjective();
    defineUnique();
    defineRest();
  }
}
void Schemes::scheme10() {
  _name = "AnnImpOsc";
  reset();
  defineAnnotated();
  defineImplicit();
  defineFromObjective();
  defineUnique();
  defineRest();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineAnnotated();
    defineImplicit();
    defineFromObjective();
    defineUnique();
    defineRest();
  }
}
void Schemes::annOnly() {
  _name = "AnnOnly\t";
  reset();
  defineAnnotated();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineAnnotated();
  }
}
void Schemes::annImp() {
  _name = "AnnImpOnly";
  reset();
  defineAnnotated();
  defineImplicit();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineAnnotated();
    defineImplicit();
  }
}

void Schemes::random() {
  _name = "Random\t";
  reset();
  defineRandom();
  while (hasCycle().size()) {
    removeCycles(true);
    updateDomains();
    defineRandom();
  }
  _m->reportDomainChange();
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
  _m->reportDomainChange();
  _m->reportPotDefChange();
}
void Schemes::updateDomains() {
  for (auto constraint : _m->constraints()) {  // _m->conMap.functional()
    if (constraint->isInvariant()) {
      std::set<Constraint*> visited;
      constraint->refreshAndPropagate(visited);
    }
  }
  _m->reportDomainChange();
}
void Schemes::defineAnnotated() {
  for (auto c : _m->constraints()) {
    if (c->annotationTarget().has_value() &&
        !c->annotationTarget().value()->isDefined() &&
        (c->annotationTarget().value()->hasPotentialDefiner(c))) {
      c->define(c->annotationTarget().value());
    }
  }
  _m->reportDomainChange();
}
void Schemes::defineImplicit() {
  for (auto c : _m->constraints()) {
    if (c->canBeImplicit() && c->allVariablesFree()) {
      c->makeImplicit();
    }
  }
  _m->reportDomainChange();
}
void Schemes::defineFromWithImplicit(Variable* variable) {
  if (variable->isDefinable() && !variable->isDefined()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->canDefine(variable) && constraint->notFull()) {
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
  _m->reportDomainChange();
}
void Schemes::defineFrom(Variable* variable) {
  if (variable->isDefinable() && !variable->isDefined()) {
    for (auto constraint : variable->potentialDefiners()) {
      if (constraint->canDefine(variable) && constraint->notFull()) {
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
  _m->reportDomainChange();
}
void Schemes::defineFromObjective() {
  if (_m->objective().has_value()) {
    defineFrom(_m->objective().value());
  } else {
    // std::cerr << "No objective exists\n";
  }
  _m->reportDomainChange();
}
void Schemes::defineUnique() {
  for (auto variable : _m->domSortVariables()) {
    if (variable->isDefinable() && !variable->isDefined()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->canDefine(variable) && constraint->uniqueTarget() &&
            constraint->notFull()) {
          constraint->define(variable);
          break;
        }
      }
    }
  }
  _m->reportDomainChange();
}
void Schemes::defineRest() {
  for (auto variable : _m->domSortVariables()) {
    if (variable->isDefinable() && !variable->isDefined()) {
      for (auto constraint : variable->potentialDefiners()) {
        if (constraint->canDefine(variable) && constraint->notFull()) {
          constraint->define(variable);
          break;
        }
      }
    }
  }
  _m->reportDomainChange();
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
  _m->reportDomainChange();
}
void Schemes::removeCycle(std::vector<Node*> visited, bool ban) {
  Int smallestDomain = std::numeric_limits<Int>::max();
  Constraint* consToRemove = nullptr;
  Variable* varToRemove = nullptr;

  for (auto node : visited) {
    if (auto v = dynamic_cast<Variable*>(node)) {
      if (v->domainSize() <= smallestDomain) {
        if (v->domainSize() < smallestDomain) {
          varToRemove = v;
          consToRemove = v->definedBy();
          smallestDomain = v->domainSize();
        } else {
          if (v->definedBy()->defInVarCount() > consToRemove->defInVarCount()) {
            varToRemove = v;
            consToRemove = v->definedBy();
            smallestDomain = v->domainSize();
          }
        }
      }
    }
  }
  if (ban) {
    assert(consToRemove->breakCycleWithBan(varToRemove));
  } else {
    assert(consToRemove->breakCycle(varToRemove));
  }
  std::cout << (TRACK ? "Node: " + consToRemove->getName() + " removed." + "\n"
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
  if (ban) _m->reportPotDefChange();
  std::cout << (TRACK ? "Done! No cycles.\n" : "");
}
std::vector<Node*> Schemes::hasCycle() {
  std::set<Node*> visited;
  std::vector<Node*> stack;
  for (auto node : _m->varMap().variables()) {
    if (hasCycleAux(visited, stack, node)) {
      if (_ignoreDynamicCycles) {
        return checkDynamicCycle(stack);
      } else {
        return stack;
      }
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
        if (con->notFull() && con->canDefine(var)) {  // Add better prio
          con->define(var);
        } else if (con->canBeImplicit() && con->allVariablesFree()) {
          con->makeImplicit();
        }
        break;
      }
    }
  }
}
void Schemes::defineRandom() {
  std::vector<Variable*> vars = _m->domSortVariables();
  std::random_shuffle(vars.begin(), vars.end());
  for (auto v : vars) {
    if (!v->isDefined()) {
      std::vector<Constraint*> cns = v->potentialDefiners();
      std::random_shuffle(cns.begin(), cns.end());
      for (auto con : cns) {
        if (con->notFull() && con->canDefine(v)) {  // Add better prio
          con->define(v);
        } else if (con->canBeImplicit() && con->allVariablesFree()) {
          con->makeImplicit();
        }
        break;
      }
    }
  }
}

std::vector<Node*> Schemes::checkDynamicCycle(std::vector<Node*> stack) {
  if (auto e = dynamic_cast<VarElement*>(stack.front())) {
    if (e->isIndexVar(stack.back())) return stack;
  }
  for (auto node : stack) {
    for (auto next : node->getNext()) {
      if (auto e = dynamic_cast<VarElement*>(next)) {
        if (e->isIndexVar(node)) return stack;
      }
    }
  }
  for (auto node : stack) {
    if (dynamic_cast<VarElement*>(node)) {
      return std::vector<Node*>();
    }
  }
  return stack;
}

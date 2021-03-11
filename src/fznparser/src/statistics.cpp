#include "statistics.hpp"

Statistics::Statistics(Model* model) { _model = model; }
int Statistics::variableCount() {
  int n = 0;
  for (auto variable : _model->variables()) {
    n += variable->count();
  }
  return n;
}
int Statistics::definedCount() {
  int n = 0;
  for (auto variable : _model->variables()) {
    if (variable->isDefined()) {
      n += variable->count();
    }
  }
  return n;
}
void Statistics::countDefinedVariables(bool labels) {
  std::cout << "=========VARIABLES=========" << std::endl;
  if (labels) {
    variablesDefinedBy();
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Defined:\t";
  std::cout << definedCount() << std::endl;
  std::cout << "Total:\t\t";
  std::cout << variableCount() << std::endl;
  std::cout << "===========================" << std::endl;
}
void Statistics::variablesDefinedBy() {
  for (auto variable : _model->variables()) {
    if (variable->isDefined()) {
      std::cout << "Var: " << variable->getLabel();
      std::cout << "\tDS: " << variable->domainSize();
      if (variable->hasImposedDomain()) {
        std::cout << "\t(imposed)";
      }
      std::cout << "\t[" << variable->definedBy()->getLabel() << "]"
                << std::endl;
    }
  }
}
void Statistics::constraints(bool labels) {
  std::cout << "========CONSTRAINTS========" << std::endl;
  int total = 0;
  int invariant = 0;
  int implicit = 0;
  for (auto constraint : _model->constraints()) {
    total++;
    if (constraint->isImplicit()) {
      implicit++;
    } else if (constraint->isInvariant()) {
      invariant++;
    }
    if (labels) {
      std::cout << constraint->getLabel() << std::endl;
    }
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Invariants:\t" << invariant << std::endl;
  std::cout << "Implicit:\t" << implicit << std::endl;
  std::cout << "Soft:\t\t" << total - invariant - implicit << std::endl;
  std::cout << "Total:\t\t" << total << std::endl;
  std::cout << "===========================" << std::endl;
}
// void Statistics::cyclesRemoved() {
//   std::cout << "Cycles removed: " << _model->cyclesRemoved() << std::endl;
//   std::cout << "===========================" << std::endl;
// }
void Statistics::allStats(bool labels) {
  // cyclesRemoved();
  countDefinedVariables(labels);
  constraints(labels);
}

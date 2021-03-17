#include "statistics.hpp"

Statistics::Statistics(Model* model) { _model = model; }
int Statistics::variableCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    n++;
  }
  return n;
}
int Statistics::definedCount() {
  int n = 0;
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefined()) {
      n++;
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
  for (auto variable : _model->varMap().variables()) {
    if (variable->isDefined()) {
      std::cout << "Var: " << variable->getName();
      std::cout << "\tDS: " << variable->domainSize();
      if (variable->hasImposedDomain()) {
        std::cout << "\t(imposed)";
      }
      std::cout << "\t[" << variable->definedBy()->getName() << "]"
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
      std::cout << constraint->getName() << std::endl;
    }
  }
  std::cout << "===========================" << std::endl;
  std::cout << "Invariants:\t" << invariant << std::endl;
  std::cout << "Implicit:\t" << implicit << std::endl;
  std::cout << "Soft:\t\t" << total - invariant - implicit << std::endl;
  std::cout << "Total:\t\t" << total << std::endl;
  std::cout << "===========================" << std::endl;
}
void Statistics::allStats(bool labels) {
  countDefinedVariables(labels);
  constraints(labels);
  // width();
}
void Statistics::width() {
  // assert(_model->hasNoCycles());
  int w = 0;
  for (Node* v : _model->varMap().variables()) {
    width_aux(v, 1, w);
  }
  for (Node* c : _model->constraints()) {
    width_aux(c, 1, w);
  }
  std::cout << "Width of graph: " << w << std::endl;
}
void Statistics::width_aux(Node* node, int x, int& w) {
  if (x > w) w = x;
  for (auto n : node->getNext()) {
    width_aux(n, x + 1, w);
  }
}

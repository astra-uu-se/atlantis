#include "statistics.hpp"

Statistics::Statistics(Model* model) { _model = model; }

void Statistics::countDefinedVariables() {
  std::cout << "Defined variable count:\t";
  std::cout << _model->definedCount() << std::endl;
  std::cout << "Total amount of variables:\t";
  std::cout << _model->variableCount() << std::endl;
}

void Statistics::variablesDefinedBy() {
  for (auto variable : _model->variables()) {
    if (variable->isDefined()) {
      std::cout << "Var: " << variable->getLabel();
      std::cout << "\tDomain size: " << variable->domainSize();
      if (variable->imposedDomain()) {
        std::cout << "\tImposed Domain size: " << variable->imposedDomainSize();
      }
      std::cout << "\t\tDefined by: " << variable->definedBy()->getLabel()
                << std::endl;
    }
  }
}

void Statistics::cyclesRemoved() {
  std::cout << "Cycles removed: " << _model->cyclesRemoved() << std::endl;
}

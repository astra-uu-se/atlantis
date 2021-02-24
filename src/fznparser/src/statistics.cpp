#include "statistics.hpp"

Statistics::Statistics(Model* model) { _model = model; }

void Statistics::countDefinedVariables() {
  std::cout << "Amount of defined (non-array) variables is:\t";
  std::cout << _model->definedCount() << std::endl;
  std::cout << "Total amount of (non-array) variables is:\t";
  std::cout << _model->variableCount() << std::endl;
}

void Statistics::variablesDefinedBy() {
  for (auto variable : _model->variables()) {
    if (variable->isDefined()) {
      std::cout << "Variable: " << variable->getLabel();
      std::cout << "\tDomain size: " << variable->domainSize();
      std::cout << "\t\tDefined by: " << variable->definedBy()->getLabel()
                << std::endl;
    }
  }
}

#include "statistics.hpp"

Statistics::Statistics(Model* model) { _model = model; }

void Statistics::checkCycles() {
  if (_model->hasCycle()) {
    std::cout << "FOUND CYCLE\n";
  } else {
    std::cout << "No cycle found\n";
  }
}

void Statistics::countDefinedVariables() {
  std::cout << "Amount of defined (non-array) variables is:\t";
  std::cout << _model->definedCount() << std::endl;
  std::cout << "Total amount of (non-array) variables is:\t";
  std::cout << _model->variableCount() << std::endl;
}

void Statistics::variablesDefinedBy() {
  for (auto v : _model->_variables) {
    auto variable = v.second.get();
    if (variable->isDefined()) {
      std::cout << "Variable: " << variable->getLabel();
      std::cout << "\tDefined by: " << variable->definedBy()->getLabel()
                << std::endl;
    }
  }
}

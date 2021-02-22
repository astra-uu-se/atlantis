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
  std::cout << "Amount of defined variables is: ";
  std::cout << _model->definedCount() << std::endl;
}

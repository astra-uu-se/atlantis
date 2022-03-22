#include "search/solutionListener.hpp"

#include <iostream>

void search::SolutionListener::onSolution(const Assignment& assignment) {
  for (const auto& variable : assignment.searchVariables()) {
    std::cout << _variableMap.at(variable)->name() << " = "
              << assignment.value(variable) << ";" << std::endl;
  }

  std::cout << "----------" << std::endl;
}

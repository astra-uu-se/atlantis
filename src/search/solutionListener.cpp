#include "search/solutionListener.hpp"

#include <iostream>

void search::SolutionListener::onSolution(const Assignment& assignment) {
  for (const auto& variable : assignment.searchVariables()) {
    // The line is ended by an explicit '\n' character, so we don't flush the
    // buffer immediately. Instead, the buffer is flushed at the end of the
    // solution output.
    std::cout << _variableMap.at(variable)->name() << " = "
              << assignment.value(variable) << ";\n";
  }

  std::cout << "----------" << std::endl;
}

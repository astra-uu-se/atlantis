#include "search/solutionListener.hpp"

#include <iostream>

typedef std::shared_ptr<fznparser::SearchVariable> FZNVar;
typedef std::shared_ptr<fznparser::VariableArray> FZNVarArray;

// The lines are ended by an explicit '\n' character, so we don't flush the
// buffer immediately. Instead, the buffer is flushed at the end of the
// solution output.

static void printSearchVariable(
    const FZNVar& searchVariable, const search::Assignment& assignment,
    const search::SolutionListener::VariableMap& variableMap) {
  std::cout << searchVariable->name() << " = "
            << assignment.value(variableMap.at(searchVariable)) << ";\n";
}

static void printVariableArray(
    const FZNVarArray& variableArray, const search::Assignment& assignment,
    const search::SolutionListener::VariableMap& variableMap) {
  std::cout << variableArray->name() << " = array1d(1.."
            << variableArray->size() << ", [";

  for (auto i = 0u; i < variableArray->size(); ++i) {
    auto searchVariable = variableArray->contents()[i];
    std::cout << assignment.value(variableMap.at(searchVariable));

    if (i < variableArray->size() - 1) {
      std::cout << ", ";
    }
  }

  std::cout << "])\n";
}

void search::SolutionListener::onSolution(const Assignment& assignment) {
  for (const auto& fznVariable : _fznModel.variables()) {
    if (fznVariable->annotations().has<fznparser::OutputAnnotation>()) {
      if (auto searchVariable =
              std::dynamic_pointer_cast<fznparser::SearchVariable>(
                  fznVariable)) {
        printSearchVariable(searchVariable, assignment, _variableMap);
      } else {
        auto variableArray =
            std::dynamic_pointer_cast<fznparser::VariableArray>(fznVariable);
        assert(variableArray);
        printVariableArray(variableArray, assignment, _variableMap);
      }
    }
  }

  std::cout << "----------" << std::endl;
}

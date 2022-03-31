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
  auto ann =
      variableArray->annotations().get<fznparser::OutputArrayAnnotation>();

  std::cout << variableArray->name() << " = array" << ann->dimensions().size()
            << "d(";
  for (auto size : ann->dimensions()) {
    std::cout << "1.." << size << ", ";
  }

  std::cout << "[";

  for (auto i = 0u; i < variableArray->size(); ++i) {
    auto searchVariable = variableArray->contents()[i];
    std::cout << assignment.value(variableMap.at(searchVariable));

    if (i < variableArray->size() - 1) {
      std::cout << ", ";
    }
  }

  std::cout << "]);\n";
}

void search::SolutionListener::onSolution(const Assignment& assignment) {
  for (const auto& fznVariable : _fznModel.variables()) {
    if (fznVariable->annotations().has<fznparser::OutputAnnotation>()) {
      printSearchVariable(
          std::dynamic_pointer_cast<fznparser::SearchVariable>(fznVariable),
          assignment, _variableMap);
    } else if (fznVariable->annotations()
                   .has<fznparser::OutputArrayAnnotation>()) {
      printVariableArray(
          std::dynamic_pointer_cast<fznparser::VariableArray>(fznVariable),
          assignment, _variableMap);
    }
  }

  std::cout << "----------" << std::endl;
}

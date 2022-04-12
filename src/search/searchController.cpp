#include "search/searchController.hpp"

#include <iostream>

#include "utils/fznAst.hpp"

bool search::SearchController::shouldRun(const search::Assignment& assignment) {
  if (assignment.satisfiesConstraints()) {
    return false;
  }

  if (_started && _timeout.has_value()) {
    return std::chrono::steady_clock::now() - _startTime <= *_timeout;
  }

  _started = true;
  _startTime = std::chrono::steady_clock::now();
  return true;
}

static void printSearchVariable(
    const fznparser::Identifier& searchVariable,
    const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap) {
  std::cout << searchVariable << " = "
            << assignment.value(variableMap.at(searchVariable)) << ";\n";
}

template <typename T>
static void printVariableArray(
    const fznparser::VariableArray<T>& variableArray,
    const fznparser::OutputArrayAnnotation& ann,
    const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap) {
  std::cout << variableArray.name << " = array" << ann.sizes.size() << "d(";
  for (auto size : ann.sizes) {
    std::cout << "1.." << size << ", ";
  }

  std::cout << "[";

  for (auto i = 0u; i < variableArray.size(); ++i) {
    auto elem = variableArray[i];
    if (std::holds_alternative<T>(elem)) {
      std::cout << std::get<T>(elem);
    } else {
      std::cout << assignment.value(
          variableMap.at(std::get<fznparser::Identifier>(elem)));
    }

    if (i < variableArray.size() - 1) {
      std::cout << ", ";
    }
  }

  std::cout << "]);\n";
}

void search::SearchController::onSolution(const Assignment& assignment) {
  for (const auto& fznVariable : _fznModel.variables()) {
    auto tagAnnotation = getAnnotation<fznparser::TagAnnotation>(fznVariable);
    auto arrayOutputAnnotation =
        getAnnotation<fznparser::OutputArrayAnnotation>(fznVariable);

    if (tagAnnotation && tagAnnotation->tag == "output_var") {
      printSearchVariable(identifier(fznVariable), assignment, _variableMap);
    } else if (arrayOutputAnnotation) {
      if (std::holds_alternative<fznparser::IntVariableArray>(fznVariable)) {
        printVariableArray<Int>(
            std::get<fznparser::IntVariableArray>(fznVariable),
            *arrayOutputAnnotation, assignment, _variableMap);
      } else if (std::holds_alternative<fznparser::BoolVariableArray>(
                     fznVariable)) {
        printVariableArray<bool>(
            std::get<fznparser::BoolVariableArray>(fznVariable),
            *arrayOutputAnnotation, assignment, _variableMap);
      }
    }
  }

  std::cout << "----------" << std::endl;
}

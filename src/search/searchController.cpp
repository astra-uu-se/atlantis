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
  std::cout << variableArray->name() << " = array" << ann.sizes.size() << "d(";
  for (auto size : ann.sizes) {
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

void search::SearchController::onSolution(const Assignment& assignment) {
  for (const auto& fznVariable : _fznModel.variables()) {
    auto tagAnnotation = getAnnotation<fznparser::TagAnnotation>(fznVariable);
    auto arrayOutputAnnotation =
        getAnnotation<fznparser::OutputArrayAnnotation>(fznVariable);

    if (tagAnnotation && tagAnnotation->tag == "output_var") {
      printSearchVariable(identifier(fznVariable), assignment, _variableMap);
    } else if (arrayOutputAnnotation) {
      std::visit(
          [&](const auto& array) {
            if constexpr (std::is_same_v<decltype(array),
                                         fznparser::IntVariableArray> ||
                          std::is_same_v<decltype(array),
                                         fznparser::BoolVariableArray>)
              printVariableArray(array, *arrayOutputAnnotation, assignment,
                                 _variableMap);
          },
          fznVariable);
    }
  }

  std::cout << "----------" << std::endl;
}

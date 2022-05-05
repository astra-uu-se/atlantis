#include "search/searchController.hpp"

#include <iostream>

#include "utils/fznAst.hpp"

bool search::SearchController::shouldRun(const search::Assignment&) {
  if (_foundSolution &&
      std::holds_alternative<fznparser::Satisfy>(_fznModel.objective())) {
    return false;
  }

  if (_started && _timeout.has_value()) {
    return std::chrono::steady_clock::now() - _startTime <= *_timeout;
  }

  _started = true;
  _startTime = std::chrono::steady_clock::now();
  return true;
}

static void printSolution(
    const fznparser::FZNModel& model, const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap);

void search::SearchController::onSolution(const Assignment& assignment) {
  assert(assignment.satisfiesConstraints());

  _foundSolution = true;
  printSolution(_fznModel, assignment, _variableMap);
}

void search::SearchController::onFinish() const {
  if (!_foundSolution) {
    std::cout << "=====UNKNOWN=====" << std::endl;
  }
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

static void printSolution(
    const fznparser::FZNModel& model, const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap) {
  for (const auto& fznVariable : model.variables()) {
    auto tagAnnotation = getAnnotation<fznparser::TagAnnotation>(fznVariable);
    auto arrayOutputAnnotation =
        getAnnotation<fznparser::OutputArrayAnnotation>(fznVariable);

    if (tagAnnotation && tagAnnotation->tag == "output_var") {
      printSearchVariable(identifier(fznVariable), assignment, variableMap);
    } else if (arrayOutputAnnotation) {
      if (std::holds_alternative<fznparser::IntVariableArray>(fznVariable)) {
        printVariableArray<Int>(
            std::get<fznparser::IntVariableArray>(fznVariable),
            *arrayOutputAnnotation, assignment, variableMap);
      } else if (std::holds_alternative<fznparser::BoolVariableArray>(
                     fznVariable)) {
        printVariableArray<bool>(
            std::get<fznparser::BoolVariableArray>(fznVariable),
            *arrayOutputAnnotation, assignment, variableMap);
      }
    }
  }

  std::cout << "----------" << std::endl;
}

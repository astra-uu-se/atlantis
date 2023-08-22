#include "search/searchController.hpp"

#include <iostream>

#include "utils/fznAst.hpp"

bool search::SearchController::shouldRun(const search::Assignment&) {
  if (_foundSolution && _model.isSatisfactionProblem()) {
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
    const fznparser::Model& model, const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap);

void search::SearchController::onSolution(const Assignment& assignment) {
  assert(assignment.satisfiesConstraints());

  _foundSolution = true;
  printSolution(_model, assignment, _variableMap);
}

void search::SearchController::onFinish() const {
  if (!_foundSolution) {
    std::cout << "=====UNKNOWN=====" << std::endl;
  }
}

static void printSearchVariable(
    const std::string_view& searchVariable,
    const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap) {
  std::cout << searchVariable << " = "
            << assignment.value(variableMap.at(searchVariable)) << ";\n";
}

std::string toString(bool b) { return b ? "true" : "false"; }
std::string toString(Int i) { return std::to_string(i); }

std::string arrayVarPrefix(const std::vector<Int>& indexSetSizes) {
  std::string s = " = array" + std::to_string(indexSetSizes.size()) + "d(";

  for (Int size : indexSetSizes) {
    s += "1.." + std::to_string(size) + ", ";
  }

  return s;
}

static void printVariableArray(
    const fznparser::BoolVarArray& variableArray,
    const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap) {
  const std::vector<Int>& outputIndexSetSizes =
      variableArray.outputIndexSetSizes();

  std::cout << variableArray.identifier() << arrayVarPrefix(outputIndexSetSizes)
            << '[';

  for (auto i = 0u; i < variableArray.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    if (std::holds_alternative<bool>(variableArray[i])) {
      std::cout << toString(std::get<bool>(variableArray[i]));
    } else {
      const fznparser::BoolVar& var =
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(
              variableArray[i])
              .get();

      if (var.isFixed()) {
        std::cout << toString(var.lowerBound());
        continue;
      }

      assert(variableMap.contains(var.identifier()));

      std::cout << toString(
          assignment.value(variableMap.at(var.identifier())) == 0);
    }
  }

  std::cout << "]);\n";
}

static void printVariableArray(
    const fznparser::IntVarArray& variableArray,
    const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap) {
  const std::vector<Int>& outputIndexSetSizes =
      variableArray.outputIndexSetSizes();

  std::cout << variableArray.identifier() << arrayVarPrefix(outputIndexSetSizes)
            << '[';

  for (auto i = 0u; i < variableArray.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    if (std::holds_alternative<Int>(variableArray[i])) {
      std::cout << std::get<Int>(variableArray[i]);
    } else {
      const fznparser::IntVar& var =
          std::get<std::reference_wrapper<const fznparser::IntVar>>(
              variableArray[i])
              .get();
      if (var.isFixed()) {
        std::cout << var.lowerBound();
        continue;
      }

      assert(variableMap.contains(var.identifier()));

      std::cout << assignment.value(variableMap.at(var.identifier()));
    }
  }

  std::cout << "]);\n";
}

static void printSolution(
    const fznparser::Model& model, const search::Assignment& assignment,
    const search::SearchController::VariableMap& variableMap) {
  for (const auto& [identifier, variable] : model.variables()) {
    if (!variable.isOutput()) {
      continue;
    }
    if (std::holds_alternative<fznparser::BoolVarArray>(variable)) {
      printVariableArray(std::get<fznparser::BoolVarArray>(variable),
                         assignment, variableMap);
    } else if (std::holds_alternative<fznparser::IntVarArray>(variable)) {
      printVariableArray(std::get<fznparser::IntVarArray>(variable), assignment,
                         variableMap);
    } else {
      printSearchVariable(identifier, assignment, variableMap);
    }
  }

  std::cout << "----------" << std::endl;
}

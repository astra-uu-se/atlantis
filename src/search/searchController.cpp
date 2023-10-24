#include "search/searchController.hpp"

namespace atlantis::search {

bool SearchController::shouldRun(const Assignment&) {
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

static void printSolution(const fznparser::Model& model,
                          const Assignment& assignment,
                          const SearchController::VarMap& varMap);

void SearchController::onSolution(const Assignment& assignment) {
  assert(assignment.satisfiesConstraints());

  _foundSolution = true;
  printSolution(_model, assignment, _varMap);
}

void SearchController::onFinish() const {
  if (!_foundSolution) {
    std::cout << "=====UNKNOWN=====" << std::endl;
  }
}

static void printSearchVar(const std::string& searchVar,
                           const Assignment& assignment,
                           const SearchController::VarMap& varMap) {
  std::cout << searchVar << " = " << assignment.value(varMap.at(searchVar))
            << ";\n";
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

static void printVarArray(const fznparser::BoolVarArray& varArray,
                          const Assignment& assignment,
                          const SearchController::VarMap& varMap) {
  const std::vector<Int>& outputIndexSetSizes = varArray.outputIndexSetSizes();

  std::cout << varArray.identifier() << arrayVarPrefix(outputIndexSetSizes)
            << '[';

  for (auto i = 0u; i < varArray.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    if (std::holds_alternative<bool>(varArray[i])) {
      std::cout << toString(std::get<bool>(varArray[i]));
    } else {
      const fznparser::BoolVar& var =
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(
              varArray[i])
              .get();

      if (var.isFixed()) {
        std::cout << toString(var.lowerBound());
        continue;
      }

      assert(varMap.contains(var.identifier()));

      std::cout << toString(assignment.value(varMap.at(var.identifier())) == 0);
    }
  }

  std::cout << "]);\n";
}

static void printVarArray(const fznparser::IntVarArray& varArray,
                          const Assignment& assignment,
                          const SearchController::VarMap& varMap) {
  const std::vector<Int>& outputIndexSetSizes = varArray.outputIndexSetSizes();

  std::cout << varArray.identifier() << arrayVarPrefix(outputIndexSetSizes)
            << '[';

  for (auto i = 0u; i < varArray.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    if (std::holds_alternative<Int>(varArray[i])) {
      std::cout << std::get<Int>(varArray[i]);
    } else {
      const fznparser::IntVar& var =
          std::get<std::reference_wrapper<const fznparser::IntVar>>(varArray[i])
              .get();
      if (var.isFixed()) {
        std::cout << var.lowerBound();
        continue;
      }

      assert(varMap.contains(var.identifier()));

      std::cout << assignment.value(varMap.at(var.identifier()));
    }
  }

  std::cout << "]);\n";
}

static void printSolution(const fznparser::Model& model,
                          const Assignment& assignment,
                          const SearchController::VarMap& varMap) {
  for (const auto& [identifier, var] : model.vars()) {
    if (!var.isOutput()) {
      continue;
    }
    if (std::holds_alternative<fznparser::BoolVarArray>(var)) {
      printVarArray(std::get<fznparser::BoolVarArray>(var), assignment, varMap);
    } else if (std::holds_alternative<fznparser::IntVarArray>(var)) {
      printVarArray(std::get<fznparser::IntVarArray>(var), assignment, varMap);
    } else {
      printSearchVar(identifier, assignment, varMap);
    }
  }

  std::cout << "----------" << std::endl;
}

}  // namespace atlantis::search
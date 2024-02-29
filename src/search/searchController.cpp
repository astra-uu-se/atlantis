#include "atlantis/search/searchController.hpp"

namespace atlantis::search {

bool SearchController::shouldRun(const Assignment&) {
  if (_foundSolution && _isSatisfactionProblem) {
    return false;
  }

  if (_started && _timeout.has_value()) {
    return std::chrono::steady_clock::now() - _startTime <= *_timeout;
  }

  _started = true;
  _startTime = std::chrono::steady_clock::now();
  return true;
}

void SearchController::onSolution(const Assignment& assignment) {
  assert(assignment.satisfiesConstraints());
  _foundSolution = true;
  printSolution(assignment);
}

void SearchController::onFinish() const {
  if (!_foundSolution) {
    std::cout << "=====UNKNOWN=====" << std::endl;
  }
}

std::string toIntString(const Assignment& assignment,
                        const std::variant<propagation::VarId, Int>& var) {
  return std::to_string(
      std::holds_alternative<Int>(var)
          ? std::get<Int>(var)
          : assignment.value(std::get<propagation::VarId>(var)));
}

std::string toBoolString(const Assignment& assignment,
                         const std::variant<propagation::VarId, Int>& var) {
  return ((std::holds_alternative<Int>(var)
               ? std::get<Int>(var)
               : assignment.value(std::get<propagation::VarId>(var))) == 0)
             ? "true"
             : "false";
}

void SearchController::printBoolVar(const Assignment& assignment,
                                    const FznOutputVar& outputVar) {
  std::cout << outputVar.identifier << " = "
            << toBoolString(assignment, outputVar.var) << ";\n";
}

void SearchController::printIntVar(const Assignment& assignment,
                                   const FznOutputVar& outputVar) {
  std::cout << outputVar.identifier << " = "
            << toIntString(assignment, outputVar.var) << ";\n";
}

std::string arrayVarPrefix(const std::vector<Int>& indexSetSizes) {
  std::string s = " = array" + std::to_string(indexSetSizes.size()) + "d(";

  for (Int size : indexSetSizes) {
    s += "1.." + std::to_string(size) + ", ";
  }

  return s;
}

void SearchController::printBoolVarArray(const Assignment& assignment,
                                         const FznOutputVarArray& varArray) {
  std::cout << varArray.identifier << arrayVarPrefix(varArray.indexSetSizes)
            << '[';

  for (size_t i = 0; i < varArray.vars.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    std::cout << toBoolString(assignment, varArray.vars[i]);
  }

  std::cout << "]);\n";
}

void SearchController::printIntVarArray(const Assignment& assignment,
                                        const FznOutputVarArray& varArray) {
  std::cout << varArray.identifier << arrayVarPrefix(varArray.indexSetSizes)
            << '[';

  for (size_t i = 0; i < varArray.vars.size(); ++i) {
    if (i != 0) {
      std::cout << ", ";
    }
    std::cout << toIntString(assignment, varArray.vars[i]);
  }

  std::cout << "]);\n";
}
void SearchController::printSolution(const Assignment& assignment) const {
  for (const auto& outputVar : _outputBoolVars) {
    printBoolVar(assignment, outputVar);
  }
  for (const auto& outputVar : _outputIntVars) {
    printIntVar(assignment, outputVar);
  }
  for (const auto& outputVarArray : _outputBoolVarArrays) {
    printBoolVarArray(assignment, outputVarArray);
  }
  for (const auto& outputVarArray : _outputIntVarArrays) {
    printIntVarArray(assignment, outputVarArray);
  }

  std::cout << "----------\n";
}

}  // namespace atlantis::search

#include "atlantis/utils/fznOutput.hpp"

namespace atlantis {

FznOutput::FznOutput(std::vector<FznOutputVar>&& boolVars,
                     std::vector<FznOutputVar>&& intVars,
                     std::vector<FznOutputVarArray>&& boolVarArrays,
                     std::vector<FznOutputVarArray>&& intVarArrays) :
_boolVars(boolVars),
_intVars(intVars),
_boolVarArrays(boolVarArrays),
_intVarArrays(intVarArrays) {}

void FznOutput::addSolverId(const FznOutputVar& var, std::vector<propagation::VarViewId>& solverIds) {
  if (std::holds_alternative<propagation::VarViewId>(var.var)) {
    solverIds.emplace_back(std::get<propagation::VarViewId>(var.var));
  }
}

void FznOutput::addSolverIds(const FznOutputVarArray& arr, std::vector<propagation::VarViewId>& solverIds) {
  for (const auto& var : arr.vars) {
    if (std::holds_alternative<propagation::VarViewId>(var)) {
      solverIds.emplace_back(std::get<propagation::VarViewId>(var));
    }
  }
}


void FznOutput::appendBoolVar(FznOutputVar&& var) {
  _boolVars.emplace_back(std::move(var));
}

void FznOutput::appendIntVar(FznOutputVar&& var) {
  _intVars.emplace_back(std::move(var));
}

void FznOutput::appendBoolVarArray(FznOutputVarArray&& arr) {
  _boolVarArrays.emplace_back(std::move(arr));
}

void FznOutput::appendIntVarArray(FznOutputVarArray&& arr) {
  _intVarArrays.emplace_back(std::move(arr));
}

std::vector<propagation::VarViewId>
FznOutput::getSolverIds() const {
  std::vector<propagation::VarViewId> solverIds;
  for (const auto& var : _boolVars) {
    addSolverId(var, solverIds);
  }
  for (const auto& arr : _boolVarArrays) {
    addSolverIds(arr, solverIds);
  }
  for (const auto& var : _intVars) {
    addSolverId(var, solverIds);
  }
  for (const auto& arr : _intVarArrays) {
    addSolverIds(arr, solverIds);
  }
  return solverIds;
}

std::string toIntString(const std::variant<propagation::VarViewId, Int>& var,
  std::vector<Int>::const_iterator& valIter) {
  return std::to_string(
      std::holds_alternative<Int>(var)
          ? std::get<Int>(var)
          : *(valIter++));
}

std::string toBoolString(const std::variant<propagation::VarViewId, Int>& var,
  std::vector<Int>::const_iterator& valIter) {
  return ((std::holds_alternative<Int>(var)
               ? std::get<Int>(var)
               : *(valIter++)) == 0)
             ? "true"
             : "false";
}

void printBoolVar(std::ostream& ostream,
                  const FznOutputVar& outputVar,
                  std::vector<Int>::const_iterator& valIter) {
  ostream << outputVar.identifier << " = "
          << toBoolString(outputVar.var, valIter) << ";\n";
}

void printIntVar(std::ostream& ostream,
                  const FznOutputVar& outputVar,
                  std::vector<Int>::const_iterator& valIter) {
  ostream << outputVar.identifier << " = "
            << toIntString(outputVar.var, valIter) << ";\n";
}

void arrayVarPrefix(std::ostream& ostream, const std::vector<Int>& indexSetSizes) {
  ostream << " = array" << indexSetSizes.size() << "d(";

  for (const Int size : indexSetSizes) {
    ostream << "1.." << std::to_string(size) << ", ";
  }
}

void printBoolVarArray(std::ostream& ostream,
                  const FznOutputVarArray& varArray,
                  std::vector<Int>::const_iterator& valIter) {
  ostream << varArray.identifier;
  arrayVarPrefix(ostream, varArray.indexSetSizes);
  ostream <<  '[';

  for (size_t i = 0; i < varArray.vars.size(); ++i) {
    if (i != 0) {
      ostream << ", ";
    }
    ostream << toBoolString(varArray.vars[i], valIter);
  }

  ostream << "]);\n";
}

void printIntVarArray(std::ostream& ostream,
                  const FznOutputVarArray& varArray,
                  std::vector<Int>::const_iterator& valIter) {
  ostream << varArray.identifier;
  arrayVarPrefix(ostream, varArray.indexSetSizes);
  ostream <<  '[';

  for (size_t i = 0; i < varArray.vars.size(); ++i) {
    if (i != 0) {
      ostream << ", ";
    }
    ostream << toIntString( varArray.vars[i], valIter);
  }

  ostream << "]);\n";
}

void FznOutput::displaySolution(std::ostream& ostream,
                                const std::vector<Int>& solverVals) const {
  auto valIter = solverVals.begin();
  for (const auto& outputVar : _boolVars) {
    printBoolVar(ostream, outputVar, valIter);
  }
  for (const auto& outputVar : _intVars) {
    printIntVar(ostream, outputVar, valIter);
  }
  for (const auto& outputVarArray : _boolVarArrays) {
    printBoolVarArray(ostream, outputVarArray, valIter);
  }
  for (const auto& outputVarArray : _intVarArrays) {
    printIntVarArray(ostream, outputVarArray, valIter);
  }
  assert(valIter == solverVals.end());
}

}  // namespace atlantis

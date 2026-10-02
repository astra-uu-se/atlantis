#include "atlantis/utils/fznOutput.hpp"

namespace atlantis {

FznOutput::FznOutput(std::vector<FznOutputVar>&& boolVars,
                     std::vector<FznOutputVar>&& intVars,
                     std::vector<FznOutputVarArray>&& boolVarArrays,
                     std::vector<FznOutputVarArray>&& intVarArrays)
    : _boolVars(boolVars),
      _intVars(intVars),
      _boolVarArrays(boolVarArrays),
      _intVarArrays(intVarArrays) {}

void addVarNodeId(const FznOutputVar& var,
                  std::vector<std::shared_ptr<VarNode>>& ids) {
  if (std::holds_alternative<std::shared_ptr<VarNode>>(var.var)) {
    ids.emplace_back(std::get<std::shared_ptr<VarNode>>(var.var));
  }
}

void addVarNodeId(const FznOutputVarArray& arr,
                  std::vector<std::shared_ptr<VarNode>>& ids) {
  for (const auto& var : arr.vars) {
    if (std::holds_alternative<std::shared_ptr<VarNode>>(var)) {
      ids.emplace_back(std::get<std::shared_ptr<VarNode>>(var));
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

std::vector<std::shared_ptr<VarNode>> FznOutput::varNodeIds() const {
  std::vector<std::shared_ptr<VarNode>> varNodes;
  for (const auto& var : _boolVars) {
    addVarNodeId(var, varNodes);
  }
  for (const auto& arr : _boolVarArrays) {
    addVarNodeId(arr, varNodes);
  }
  for (const auto& var : _intVars) {
    addVarNodeId(var, varNodes);
  }
  for (const auto& arr : _intVarArrays) {
    addVarNodeId(arr, varNodes);
  }
  return varNodes;
}

std::string toIntString(const std::variant<std::shared_ptr<VarNode>, Int>& var,
                        std::vector<Int>::const_iterator& valIter) {
  return std::to_string(std::holds_alternative<Int>(var) ? std::get<Int>(var)
                                                         : *(valIter++));
}

std::string toBoolString(const std::variant<std::shared_ptr<VarNode>, Int>& var,
                         std::vector<Int>::const_iterator& valIter) {
  return ((std::holds_alternative<Int>(var) ? std::get<Int>(var)
                                            : *(valIter++)) == 0)
             ? "true"
             : "false";
}

void printBoolVar(std::ostream& ostream, const FznOutputVar& outputVar,
                  std::vector<Int>::const_iterator& valIter) {
  ostream << outputVar.identifier << " = "
          << toBoolString(outputVar.var, valIter) << ";\n";
}

void printIntVar(std::ostream& ostream, const FznOutputVar& outputVar,
                 std::vector<Int>::const_iterator& valIter) {
  ostream << outputVar.identifier << " = "
          << toIntString(outputVar.var, valIter) << ";\n";
}

void arrayVarPrefix(std::ostream& ostream,
                    const std::vector<Int>& indexSetSizes) {
  ostream << " = array" << indexSetSizes.size() << "d(";

  for (const Int size : indexSetSizes) {
    ostream << "1.." << std::to_string(size) << ", ";
  }
}

void printBoolVarArray(std::ostream& ostream, const FznOutputVarArray& varArray,
                       std::vector<Int>::const_iterator& valIter) {
  ostream << varArray.identifier;
  arrayVarPrefix(ostream, varArray.indexSetSizes);
  ostream << '[';

  for (size_t i = 0; i < varArray.vars.size(); ++i) {
    if (i != 0) {
      ostream << ", ";
    }
    ostream << toBoolString(varArray.vars[i], valIter);
  }

  ostream << "]);\n";
}

void printIntVarArray(std::ostream& ostream, const FznOutputVarArray& varArray,
                      std::vector<Int>::const_iterator& valIter) {
  ostream << varArray.identifier;
  arrayVarPrefix(ostream, varArray.indexSetSizes);
  ostream << '[';

  for (size_t i = 0; i < varArray.vars.size(); ++i) {
    if (i != 0) {
      ostream << ", ";
    }
    ostream << toIntString(varArray.vars[i], valIter);
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

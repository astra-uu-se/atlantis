#include "minizincLogger.hpp"

#include <iostream>

void MiniZincLogger::solution(const search::Assignment& assignment) {
  for (const auto& [variable, varId] : _variableMap) {
    // TODO: Output arrays
    assert(variable->type() == fznparser::LiteralType::SEARCH_VARIABLE);
    auto searchVar =
        std::dynamic_pointer_cast<fznparser::SearchVariable>(variable);

    Int value = assignment.getValue(varId);
    std::cout << searchVar->name() << " = " << value << ";" << std::endl;
  }

  std::cout << "----------" << std::endl;
  _loggedSolution = true;
}

void MiniZincLogger::finish(MiniZincLogger::FinishReason reason) {
  // Since we're doing local search, the search will never be exhaustive.
  // However, have this interface for completeness' sake.
  assert(reason == FinishReason::Terminated);

  if (!_loggedSolution) {
    std::cout << "=====UNKNOWN=====" << std::endl;
  }
}

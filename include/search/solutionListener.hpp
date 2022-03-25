#pragma once

#include <map>

#include "assignment.hpp"
#include "core/types.hpp"
#include "fznparser/model.hpp"
#include "misc/logging.hpp"

namespace search {

class SolutionListener {
 public:
  using VariableMap =
      std::map<VarId, std::shared_ptr<fznparser::SearchVariable>>;

 private:
  VariableMap _variableMap;

 public:
  explicit SolutionListener(VariableMap variableMap)
      : _variableMap(std::move(variableMap)) {}

  void onSolution(const Assignment& assignment);
};

}  // namespace search
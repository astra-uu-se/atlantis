#pragma once

#include <unordered_map>

#include "assignment.hpp"
#include "core/types.hpp"
#include "fznparser/model.hpp"
#include "misc/logging.hpp"

namespace search {

class SolutionListener {
 public:
  using VariableMap =
      std::unordered_map<std::shared_ptr<fznparser::SearchVariable>, VarId>;

 private:
  const fznparser::Model& _fznModel;
  VariableMap _variableMap;

 public:
  explicit SolutionListener(const fznparser::Model& fznModel,
                            VariableMap variableMap)
      : _fznModel(fznModel), _variableMap(std::move(variableMap)) {}

  void onSolution(const Assignment& assignment);
};

}  // namespace search
#pragma once

#include <chrono>

#include "assignment.hpp"
#include "fznparser/model.hpp"

namespace atlantis::search {

class SearchController {
 public:
  using VariableMap = std::unordered_map<std::string, propagation::VarId>;

 private:
  const fznparser::Model& _model;
  VariableMap _variableMap;
  std::optional<std::chrono::milliseconds> _timeout;

  std::chrono::steady_clock::time_point _startTime;
  bool _started{false};
  Int _foundSolution{false};

 public:
  SearchController(const fznparser::Model& model, VariableMap variableMap)
      : _model(model), _variableMap(std::move(variableMap)), _timeout({}) {}

  template <typename Rep, typename Period>
  SearchController(const fznparser::Model& model, VariableMap variableMap,
                   std::chrono::duration<Rep, Period> timeout)
      : _model(model),
        _variableMap(std::move(variableMap)),
        _timeout(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout)) {}

  bool shouldRun(const Assignment& assignment);
  void onSolution(const Assignment& assignment);
  void onFinish() const;
};

}  // namespace search
#pragma once

#include <chrono>

#include "assignment.hpp"
#include "fznparser/model.hpp"

namespace search {

class SearchController {
 public:
  using VariableMap = std::unordered_map<std::string_view, VarId>;

 private:
  const fznparser::Model& _Model;
  VariableMap _variableMap;
  std::optional<std::chrono::milliseconds> _timeout;

  std::chrono::steady_clock::time_point _startTime;
  bool _started{false};
  Int _foundSolution{false};

 public:
  SearchController(const fznparser::Model& Model, VariableMap variableMap)
      : _Model(Model), _variableMap(std::move(variableMap)), _timeout({}) {}

  template <typename Rep, typename Period>
  SearchController(const fznparser::Model& Model, VariableMap variableMap,
                   std::chrono::duration<Rep, Period> timeout)
      : _Model(Model),
        _variableMap(std::move(variableMap)),
        _timeout(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout)) {}

  bool shouldRun(const Assignment& assignment);
  void onSolution(const Assignment& assignment);
  void onFinish() const;
};

}  // namespace search
#pragma once

#include <chrono>

#include "assignment.hpp"
#include "fznparser/model.hpp"

namespace search {

class SearchController {
 public:
  using VariableMap = std::unordered_map<fznparser::Identifier, VarId>;

 private:
  const fznparser::FZNModel& _fznModel;
  VariableMap _variableMap;
  std::optional<std::chrono::milliseconds> _timeout;

  std::chrono::steady_clock::time_point _startTime;
  bool _started{false};
  Int _foundSolution{false};

 public:
  SearchController(const fznparser::FZNModel& fznModel, VariableMap variableMap)
      : _fznModel(fznModel),
        _variableMap(std::move(variableMap)),
        _timeout({}) {}

  template <typename Rep, typename Period>
  SearchController(const fznparser::FZNModel& fznModel, VariableMap variableMap,
                   std::chrono::duration<Rep, Period> timeout)
      : _fznModel(fznModel),
        _variableMap(std::move(variableMap)),
        _timeout(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout)) {}

  bool shouldRun(const Assignment& assignment);
  void onSolution(const Assignment& assignment);
  void onFinish() const;
};

}  // namespace search
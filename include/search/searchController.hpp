#pragma once

#include <chrono>
#include <iostream>

#include "assignment.hpp"
#include "fznparser/model.hpp"
#include "utils/fznAst.hpp"

namespace atlantis::search {

class SearchController {
 public:
  using VarMap = std::unordered_map<std::string, propagation::VarId>;

 private:
  const fznparser::Model& _model;
  VarMap _varMap;
  std::optional<std::chrono::milliseconds> _timeout;

  std::chrono::steady_clock::time_point _startTime;
  bool _started{false};
  Int _foundSolution{false};

 public:
  SearchController(const fznparser::Model& model, VarMap varMap)
      : _model(model), _varMap(std::move(varMap)), _timeout({}) {}

  template <typename Rep, typename Period>
  SearchController(const fznparser::Model& model, VarMap varMap,
                   std::chrono::duration<Rep, Period> timeout)
      : _model(model),
        _varMap(std::move(varMap)),
        _timeout(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout)) {}

  bool shouldRun(const Assignment& assignment);
  void onSolution(const Assignment& assignment);
  void onFinish() const;
};

}  // namespace atlantis::search
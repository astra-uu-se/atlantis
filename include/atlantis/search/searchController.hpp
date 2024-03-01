#pragma once

#include <chrono>
#include <vector>
#include <functional>
#include <optional>

#include "atlantis/search/assignment.hpp"

namespace atlantis::search {

class SearchController {
 private:
  std::function<void(const Assignment&)> _onSolution;
  std::function<void(bool)> _onFinish;
  std::optional<std::chrono::milliseconds> _timeout;

  std::chrono::steady_clock::time_point _startTime;
  bool _isSatisfactionProblem;
  bool _started{false};
  Int _foundSolution{false};

 public:
  template <typename Rep, typename Period>
  SearchController(
      bool isSatisfactionProblem,
      std::function<void(const Assignment&)>&& onSolution,
      std::function<void(bool)>&& onFinish,
      std::optional<std::chrono::duration<Rep, Period>> timeout = {})
      : _onSolution(std::move(onSolution)),
        _onFinish(std::move(onFinish)),
        _timeout(
            timeout.has_value()
                ? std::optional<std::chrono::milliseconds>(
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                          *timeout))
                : std::nullopt),
        _isSatisfactionProblem(isSatisfactionProblem) {}

  bool shouldRun(const Assignment&);
  void onSolution(const Assignment&);
  void onFinish() const;
};

}  // namespace atlantis::search

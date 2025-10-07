#pragma once

#include <chrono>
#include <functional>
#include <optional>

#include "atlantis/types.hpp"
#include "savedAssignment.hpp"
#include "threadController.hpp"

namespace atlantis::search {

class Assignment;

class SearchController {
  std::function<void(const SavedAssignment&)> _onSolution;
  std::function<void(bool)> _onFinish;
  std::optional<std::chrono::milliseconds> _timeout;
  std::shared_ptr<const bool> _shouldStop;

  std::chrono::steady_clock::time_point _startTime;
  const bool _isSatisfactionProblem;
  bool _started{false};
  Int _foundSolution{false};

  std::shared_ptr<ThreadController> _threadController;

 public:
  template <typename Rep, typename Period>
  explicit SearchController(
      const bool isSatisfactionProblem,
      std::function<void(const SavedAssignment&)>&& onSolution,
      std::function<void(bool)>&& onFinish,
      std::optional<std::chrono::duration<Rep, Period>> timeout,
      std::shared_ptr<const bool>& shouldStop,
      const std::shared_ptr<ThreadController>& threadController)
      : _onSolution(std::move(onSolution)),
        _onFinish(std::move(onFinish)),
        _timeout(
            timeout.has_value()
                ? std::optional<std::chrono::milliseconds>(
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                          *timeout))
                : std::nullopt),
        _shouldStop(shouldStop),
        _isSatisfactionProblem(isSatisfactionProblem),
        _threadController(threadController) {}

  bool shouldRun(const Assignment&);
  void onSolution(const SavedAssignment&);
  void onFinish() const;
};

}  // namespace atlantis::search

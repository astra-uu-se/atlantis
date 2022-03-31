#pragma once

#include <chrono>

#include "assignment.hpp"

namespace search {

class SearchController {
 private:
  std::chrono::steady_clock::time_point _startTime;
  std::optional<std::chrono::milliseconds> _timeout;
  bool _started{false};

 public:
  SearchController() : _timeout({}) {}

  template <typename Rep, typename Period>
  explicit SearchController(std::chrono::duration<Rep, Period> timeout)
      : _timeout(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout)) {}

  bool shouldRun(const Assignment& assignment) {
    if (assignment.satisfiesConstraints()) {
      return false;
    }

    if (_started && _timeout.has_value()) {
      return std::chrono::steady_clock::now() - _startTime <= *_timeout;
    }

    _started = true;
    _startTime = std::chrono::steady_clock::now();
    return true;
  }
};

}  // namespace search
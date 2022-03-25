#pragma once

#include <chrono>

#include "assignment.hpp"

namespace search {

class SearchController {
 public:
  template <typename Rep, typename Period>
  explicit SearchController(std::chrono::duration<Rep, Period> timeout)
      : _timeout(
            std::chrono::duration_cast<std::chrono::milliseconds>(timeout)) {}

  bool shouldRun(const Assignment& assignment) {
    if (assignment.satisfiesConstraints()) {
      return false;
    }
    
    if (_started) {
      return std::chrono::steady_clock::now() - _startTime <= _timeout;
    }

    _started = true;
    _startTime = std::chrono::steady_clock::now();
    return true;
  }

 private:
  std::chrono::steady_clock::time_point _startTime;
  std::chrono::milliseconds _timeout;
  bool _started{false};
};

}  // namespace search
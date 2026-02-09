#include "atlantis/search/searchController.hpp"

#include <utility>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis::search {

bool SearchController::shouldRun(const Assignment& assignment) {
  if (_shouldStop != nullptr && *_shouldStop) {
    return false;
  }
  if (_isSatisfactionProblem && _threadController->hasNoViolations()) {
    return false;
  }
  if (_started && _timeout.has_value()) {
    return std::chrono::steady_clock::now() - _startTime <= *_timeout;
  }

  _started = true;
  _startTime = std::chrono::steady_clock::now();
  return true;
}

}  // namespace atlantis::search

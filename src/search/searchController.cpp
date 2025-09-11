#include "atlantis/search/searchController.hpp"

#include <utility>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis::search {

bool SearchController::shouldRun(const Assignment& assignment) {
  if (_foundSolution &&
      (_isSatisfactionProblem || assignment.objectiveIsOptimal())) {
    return false;
  }

  if (_started && _timeout.has_value()) {
    return std::chrono::steady_clock::now() - _startTime <= *_timeout;
  }

  _started = true;
  _startTime = std::chrono::steady_clock::now();
  return true;
}

SavedAssignment SearchController::onSolution(const Assignment& assignment) {
  _foundSolution = true;
  return _onSolution(assignment);
}

void SearchController::onFinish() const { _onFinish(_foundSolution); }

}  // namespace atlantis::search

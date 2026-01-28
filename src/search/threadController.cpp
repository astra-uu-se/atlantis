#include "atlantis/search/threadController.hpp"

#include <iostream>

namespace atlantis::search {

void ThreadController::setBestSolution(const Int threadId,
                                       const SavedAssignment& solution) {
  _bestThread = threadId;
  _bestCost = solution.getCost();
  _solution = solution;

  if (!_hasNoViolations) {
    if (_bestCost->getViolation() == 0) {
      _hasNoViolations = true;
    } else {
      return;
    }
  }

  ++_curSolutionId;
  _curSolutionNotified = false;
  _curSolutionNotified.notify_one();
}

bool ThreadController::trySolution(
    const Int threadId, const SavedAssignment& solution,
    const std::optional<std::shared_ptr<CounterStatistic>>&
        improvingSolutions) {
  std::lock_guard lock(_lock);

  ++_counter;

  if (!_hasSolution) {
    setBestSolution(threadId, solution);
    _hasSolution = true;
    if (improvingSolutions.has_value()) improvingSolutions.value()->increment();
    return true;
  }

  if (solution.getCost().isBetterThan(_bestCost.value()) &&
      solution.getCost().isStrictlyBetterThan(_bestCost.value())) {
    setBestSolution(threadId, solution);
    if (improvingSolutions.has_value()) improvingSolutions.value()->increment();
    return true;
  }
  return false;
}

// NOTE: this implementation is possibly not the best. This function is only
// called after the search-threads finish, so it is fine, but if we want to
// use it within the search-threads it should be changed to make _bestThread
// be atomic to avoid needing the lock. Such an implementation would make
// submitting a solution slightly less effective though, which is why it isn't
// used now.
Int ThreadController::bestThreadId() const {
  std::lock_guard lock(_lock);
  return _bestThread;
}

Cost ThreadController::cost() const {
  std::lock_guard lock(_lock);
  return _bestCost.value();
}

SavedAssignment ThreadController::solution() const {
  std::lock_guard lock(_lock);
  return _solution.value();
}

std::optional<std::pair<size_t, SavedAssignment>>
ThreadController::loadSolution(size_t solutionId) const {
  std::lock_guard lock(_lock);
  if (solutionId >= _curSolutionId || !_solution.has_value()) {
    return {};
  }
  return std::make_pair(_curSolutionId.load(), _solution.value());
}

void ThreadController::threadIsDone() {
  ++_numFinishedThreads;

  // This tells the main thread the search is done;
  if (_numFinishedThreads == _threadCount) {
    _curSolutionNotified = false;
    _curSolutionNotified.notify_one();
  }
}

}  // namespace atlantis::search

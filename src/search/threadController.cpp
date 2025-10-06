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
      _hasNoViolations.operator=(true);
    } else {
      return;
    }
  }

  _solutionNumber.operator++();
  _checkPrint.operator=(false);
  _checkPrint.notify_one();
}

bool ThreadController::trySolution(const Int threadId,
                                   const SavedAssignment& solution) {
  std::lock_guard lock(_lock);

  _counter++;

  if (!_hasSolution) {
    setBestSolution(threadId, solution);
    _hasSolution.operator=(true);
    std::cerr << _counter << ": Thread " << threadId
              << " has found the first solution with cost "
              << solution.getCost().toString() << "." << std::endl;
    return true;
  }

  if (solution.getCost().isBetterThan(_bestCost.value())) {
    if (solution.getCost().isStrictlyBetterThan(_bestCost.value())) {
      std::cerr << _counter << ": Thread " << threadId
                << " has found improving solution with cost "
                << solution.getCost().toString() << ". Previous best has cost "
                << _bestCost->toString() << "." << std::endl;
      setBestSolution(threadId, solution);
    } else {
      std::cerr << _counter << ": Thread " << threadId
                << " has found new solution with cost "
                << solution.getCost().toString() << ". Previous best has cost "
                << _bestCost->toString() << "." << std::endl;
    }
    return true;
  }

  std::cerr << _counter << ": Thread " << threadId
            << " has found new solution with cost "
            << solution.getCost().toString() << ". Previous best has cost "
            << _bestCost->toString() << "." << std::endl;

  return false;
}

// NOTE: this implementation is possibly not the best. This function is only
// called after the search-threads finish, so it is fine, but if we want to
// use it within the search-threads it should be changed to make _bestThread
// be atomic to avoid needing the lock. Such an implementation would make
// submitting a solution slightly less effective though, which is why it isn't
// used now.
Int ThreadController::getBestThreadId() const {
  std::lock_guard lock(_lock);
  return _bestThread;
}

Cost ThreadController::getCost() const {
  std::lock_guard lock(_lock);
  return _bestCost.value();
}

SavedAssignment ThreadController::getSolution() const {
  std::lock_guard lock(_lock);
  return _solution.value();
}

std::optional<std::pair<size_t, SavedAssignment>>
ThreadController::getNewerSolution(size_t solutionNumber) const {
  std::lock_guard lock(_lock);
  if (solutionNumber >= _solutionNumber || !_solution.has_value()) return {};
  return std::make_pair(_solutionNumber.load(), _solution.value());
}

void ThreadController::threadIsDone() {
  _numFinishedThreads.operator++();

  // This tells the main thread the search is done;
  if (_numFinishedThreads == _threadCount) {
    _checkPrint.operator=(false);
    _checkPrint.notify_one();
  }
}

}  // namespace atlantis::search

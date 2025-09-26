#include "atlantis/search/threadController.hpp"

#include <iostream>

namespace atlantis::search {

bool ThreadController::trySolution(const Int threadId,
                                   const SavedAssignment& solution) {
  std::lock_guard lock(_lock);

  _counter++;

  if (!_hasSolution) {
    _hasSolution = true;
    _bestThread = threadId;
    _bestCost = solution.getCost();
    _solution = solution;
    std::cerr << _counter << ": Thread " << threadId
              << " has found the first solution with cost "
              << solution.getCost().toString() << "." << std::endl;
    return true;
  }

  std::cerr << _counter << ": Thread " << threadId
            << " has found new solution with cost "
            << solution.getCost().toString() << ". Previous best has cost "
            << _bestCost->toString() << "." << std::endl;

  if (solution.getCost().isBetterThan(_bestCost.value())) {
    _bestThread = threadId;
    _bestCost = solution.getCost();
    _solution = solution;
    return true;
  }

  return false;
}

bool ThreadController::shouldPrint(const Int threadId) const {
  std::lock_guard lock(_lock);

  const bool print = _hasSolution && _bestThread == threadId;
  if (print) _printLock.lock();
  return print;
}

void ThreadController::hasPrinted() const { _printLock.unlock(); }

}  // namespace atlantis::search

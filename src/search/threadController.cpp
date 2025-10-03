#include "atlantis/search/threadController.hpp"

#include <iostream>

namespace atlantis::search {

void ThreadController::setBestSolution(const Int threadId,
                                       const SavedAssignment& solution) {
  _bestThread = threadId;
  _bestCost = solution.getCost();
  _solution = solution;

  _counterSet++;
  std::cerr << _counterSet << ": Thread " << threadId
            << " has found an improvement." << std::endl;

  if (!_hasNoViolations) {
    if (_bestCost->getViolation() == 0) {
      _hasNoViolations.operator=(true);
    } else {
      return;
    }
  }

  _counterSetSolutions++;
  std::cerr << _counterSetSolutions << ": Thread " << threadId
            << " has found an improving solution." << std::endl;

  _hasPrinted.operator=(false);
  _hasPrintedFinal.operator=(false);
  _hasPrinted.notify_one();
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

  std::cerr << _counter << ": Thread " << threadId
            << " has found new solution with cost "
            << solution.getCost().toString() << ". Previous best has cost "
            << _bestCost->toString() << "." << std::endl;

  if (solution.getCost().isBetterThan(_bestCost.value())) {
    if (solution.getCost().isStrictlyBetterThan(_bestCost.value()))
      setBestSolution(threadId, solution);
    return true;
  }

  return false;
}

Int ThreadController::getBestThreadId() const {
  // TODO: make this atomic instead of locking
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

void ThreadController::threadIsDone() {
  _numFinishedThreads.fetch_add(1);

  // This tells the main thread the search is done;
  if (_numFinishedThreads == _threadCount) {
    _hasPrinted.operator=(false);
    _hasPrinted.notify_one();
  }
}

}  // namespace atlantis::search

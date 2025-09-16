#include "atlantis/search/threadController.hpp"

#include <iostream>

namespace atlantis::search {

ThreadController::ThreadController()
    : _hasSolution(false), _bestThread(-1), _counter(-1) {}

Cost ThreadController::trySolution(const Int threadId, Cost cost) {
  std::lock_guard lock(_lock);

  _counter++;

  if (!_hasSolution) {
    _hasSolution = true;
    _bestThread = threadId;
    _bestCost = cost;
    std::cerr << _counter << ": Thread " << threadId
              << " has found the first solution with cost " << cost.toString()
              << "." << std::endl;
    return _bestCost.value();
  }

  std::cerr << _counter << ": Thread " << threadId
            << " has found new solution with cost " << cost.toString()
            << ". Previous best has cost " << _bestCost->toString() << "."
            << std::endl;

  if (cost.isBetterThan(_bestCost.value())) {
    _bestCost = cost;
    _bestThread = threadId;
  }

  return _bestCost.value();
}

bool ThreadController::shouldPrint(const Int threadId) const {
  std::lock_guard lock(_lock);

  const bool print = _hasSolution && _bestThread == threadId;
  if (print) _printLock.lock();
  return print;
}

void ThreadController::hasPrinted() const { _printLock.unlock(); }

}  // namespace atlantis::search

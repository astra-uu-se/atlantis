#include "atlantis/search/threadController.hpp"

#include <iostream>

namespace atlantis::search {

ThreadController::ThreadController()
    : _hasSolution(false), _bestThread(-1), _counter(-1) {}

void ThreadController::lock(Int threadId) const {
  // std::cerr << "Thread " << threadId << " wants the lock" << std::endl;
  _lock.lock();
  // std::cerr << "Thread " << threadId << " has taken the lock" << std::endl;
}

void ThreadController::unlock(Int threadId) const {
  _lock.unlock();
  // std::cerr << "Thread " << threadId << " has released the lock." <<
  // std::endl;
}

Cost ThreadController::trySolution(Int threadId, Cost cost) {
  lock(threadId);

  _counter++;

  if (!_hasSolution) {
    _hasSolution = true;
    _bestThread = threadId;
    _bestCost = cost;
    std::cerr << _counter << ": Thread " << threadId
              << " has found the first solution with cost " << cost.toString()
              << "." << std::endl;
    unlock(threadId);
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

  unlock(threadId);
  return _bestCost.value();
}

}  // namespace atlantis::search

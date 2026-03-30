#include "atlantis/search/threadController.hpp"

#include <iostream>
#include <sstream>

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

bool ThreadController::trySolution(const Int threadId,
                                   const SavedAssignment& solution) {
  std::lock_guard lock(_lock);

  ++_counter;

  if (!_hasSolution) {
    setBestSolution(threadId, solution);
    _hasSolution = true;
    return true;
  }

  if (solution.getCost().isBetterThan(_bestCost.value()) &&
      solution.getCost().isStrictlyBetterThan(_bestCost.value())) {
    setBestSolution(threadId, solution);
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

void ThreadController::recordFatalError(std::exception_ptr error,
                                        const Int threadId,
                                        std::string_view context) {
  if (error == nullptr) {
    return;
  }
  {
    std::lock_guard lock(_lock);
    if (_fatalError != nullptr) {
      return;
    }
    _fatalError = error;
    _fatalErrorThreadId = threadId;
    _fatalErrorContext = std::string(context);
  }
  requestStop();
}

void ThreadController::requestStop() {
  _stopRequested = true;
  _curSolutionNotified = false;
  _curSolutionNotified.notify_one();
}

bool ThreadController::stopRequested() const { return _stopRequested.load(); }

bool ThreadController::hasFatalError() const {
  std::lock_guard lock(_lock);
  return _fatalError != nullptr;
}

void ThreadController::rethrowFatalErrorIfAny() const {
  std::exception_ptr fatalError;
  std::optional<Int> threadId;
  std::optional<std::string> context;
  {
    std::lock_guard lock(_lock);
    fatalError = _fatalError;
    threadId = _fatalErrorThreadId;
    context = _fatalErrorContext;
  }
  if (fatalError == nullptr) {
    return;
  }

  try {
    std::rethrow_exception(fatalError);
  } catch (const std::exception& e) {
    std::ostringstream output;
    output << "Thread "
           << (threadId.has_value() ? std::to_string(threadId.value()) : "?");
    if (context.has_value() && !context->empty()) {
      output << " (" << context.value() << ")";
    }
    output << ": " << e.what();
    throw std::runtime_error(output.str());
  } catch (...) {
    std::ostringstream output;
    output << "Thread "
           << (threadId.has_value() ? std::to_string(threadId.value()) : "?");
    if (context.has_value() && !context->empty()) {
      output << " (" << context.value() << ")";
    }
    output << ": unknown non-standard exception";
    throw std::runtime_error(output.str());
  }
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

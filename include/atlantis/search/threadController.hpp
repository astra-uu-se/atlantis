#pragma once
#include <atomic>
#include <exception>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "cost.hpp"
#include "savedAssignment.hpp"

namespace atlantis::search {

class ThreadController {
  mutable std::mutex _lock;
  mutable std::mutex _printLock;
  Int _bestThread = -1;
  size_t _threadCount;
  std::atomic<bool> _hasSolution = false;
  std::atomic<bool> _hasNoViolations = false;
  std::atomic<bool> _curSolutionNotified = true;
  std::atomic<size_t> _curSolutionId = 0;
  std::atomic<size_t> _numFinishedThreads = 0;
  std::optional<Cost> _bestCost;
  std::optional<SavedAssignment> _solution;
  std::exception_ptr _fatalError;
  std::optional<Int> _fatalErrorThreadId;
  std::optional<std::string> _fatalErrorContext;

  // These are just for statistical tracking purposes
  [[maybe_unused]] Int _counter = 0;
  [[maybe_unused]] Int _counterSet = 0;
  [[maybe_unused]] Int _counterSetSolutions = 0;

  void setBestSolution(Int threadId, const SavedAssignment& solution);

 public:
  explicit ThreadController(const size_t threadCount)
      : _threadCount(threadCount) {}

  // Returns true iff the new solution is >= the best saved solution.
  bool trySolution(Int threadId, const SavedAssignment& solution);

  [[nodiscard]] Int bestThreadId() const;

  [[nodiscard]] Cost cost() const;

  [[nodiscard]] SavedAssignment solution() const;

  [[nodiscard]] std::optional<std::pair<size_t, SavedAssignment>> loadSolution(
      size_t solutionId) const;

  void recordFatalError(std::exception_ptr error, Int threadId,
                        std::string_view context);

  [[nodiscard]] bool hasFatalError() const;

  void rethrowFatalErrorIfAny() const;

  [[gnu::always_inline]] [[nodiscard]] bool hasSolution() const {
    return _hasSolution.load();
  }

  [[gnu::always_inline]] [[nodiscard]] bool hasNoViolations() const {
    return _hasNoViolations.load();
  }

  [[gnu::always_inline]] [[nodiscard]] size_t numFinishedThreads() const {
    return _numFinishedThreads.load();
  }

  [[gnu::always_inline]] [[nodiscard]] size_t solutionId() const {
    return _curSolutionId.load();
  }

  void threadIsDone();

  [[gnu::always_inline]] void awaitChanges() const {
    _curSolutionNotified.wait(true);
  }

  [[gnu::always_inline]] void markCurSolutionNotified() {
    _curSolutionNotified = true;
  }
};

}  // namespace atlantis::search

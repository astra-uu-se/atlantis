#pragma once
#include <atomic>
#include <mutex>
#include <optional>

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
  std::atomic<bool> _hasPrinted = true;
  std::atomic<bool> _hasPrintedFinal =
      true;  // Solution to ensure the final solution is printed exactly once.
  std::atomic<size_t> _numFinishedThreads = 0;
  std::optional<Cost> _bestCost;
  std::optional<SavedAssignment> _solution;

  // These are just for statistical tracking purposes
  Int _counter = 0;
  Int _counterSet = 0;
  Int _counterSetSolutions = 0;

  void setBestSolution(Int threadId, const SavedAssignment& solution);

 public:
  explicit ThreadController(const size_t threadCount)
      : _threadCount(threadCount) {}

  // Returns true iff the new solution is >= the best saved solution.
  bool trySolution(Int threadId, const SavedAssignment& solution);

  [[nodiscard]] Int getBestThreadId() const;

  [[nodiscard]] Cost getCost() const;

  [[nodiscard]] SavedAssignment getSolution() const;

  [[gnu::always_inline]] [[nodiscard]] bool hasSolution() const {
    return _hasSolution.load();
  }

  [[gnu::always_inline]] [[nodiscard]] bool hasNoViolations() const {
    return _hasNoViolations.load();
  }

  [[gnu::always_inline]] [[nodiscard]] size_t getNumFinishedThreads() const {
    return _numFinishedThreads.load();
  }

  [[gnu::always_inline]] [[nodiscard]] bool hasPrintedFinal() const {
    return _hasPrintedFinal.load();
  }

  void threadIsDone();

  // Wait for _hasPrinted to notify and NOT equal true
  [[gnu::always_inline]] void awaitChanges() const { _hasPrinted.wait(true); }

  [[gnu::always_inline]] void solutionPrinted() {
    _hasPrinted.operator=(true);
    _hasPrintedFinal.operator=(true);
  }
};

}  // namespace atlantis::search

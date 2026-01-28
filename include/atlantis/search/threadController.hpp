#pragma once
#include <atomic>
#include <mutex>
#include <optional>

#include "cost.hpp"
#include "savedAssignment.hpp"
#include "searchStatistics.hpp"

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
  std::atomic<size_t> _numThreadsWithReportedStats = 0;
  std::optional<Cost> _bestCost;
  std::optional<SavedAssignment> _solution;
  std::vector<std::shared_ptr<SearchStatistics>> _threadStatistics;

  // These are just for statistical tracking purposes
  Int _counter = 0;
  Int _counterSet = 0;
  Int _counterSetSolutions = 0;

  void setBestSolution(Int threadId, const SavedAssignment& solution);

 public:
  explicit ThreadController(const size_t threadCount)
      : _threadCount(threadCount) {
    _threadStatistics.resize(threadCount);
  }

  // Returns true iff the new solution is >= the best saved solution.
  bool trySolution(Int threadId, const SavedAssignment& solution,
                   const std::optional<std::shared_ptr<CounterStatistic>>&
                       improvingSolutions);

  [[nodiscard]] Int bestThreadId() const;

  [[nodiscard]] Cost cost() const;

  [[nodiscard]] SavedAssignment solution() const;

  [[nodiscard]] std::optional<std::pair<size_t, SavedAssignment>> loadSolution(
      size_t solutionId) const;

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

  [[gnu::always_inline]] [[nodiscard]] std::optional<std::vector<
      std::shared_ptr<SearchStatistics>>>
  getStats() const {
    if (_numThreadsWithReportedStats.load() >= _threadCount) return _threadStatistics;
    return std::nullopt;
  }

  [[gnu::always_inline]] void setThreadStats(
      const size_t threadId, const std::shared_ptr<SearchStatistics>& stats) {
    _threadStatistics[threadId] = stats;
    _numThreadsWithReportedStats.operator++();
  }
};

}  // namespace atlantis::search

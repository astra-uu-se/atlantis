#pragma once
#include <mutex>
#include <optional>

#include "cost.hpp"
#include "savedAssignment.hpp"

namespace atlantis::search {

class ThreadController {
  bool _hasSolution;
  std::optional<Cost> _bestCost;
  std::optional<SavedAssignment> _solution;
  Int _bestThread;
  mutable std::mutex _lock;
  mutable std::mutex _printLock;

  Int _counter;  // This is just for tracking purposes

 public:
  explicit ThreadController()
      : _hasSolution(false), _bestThread(-1), _counter(-1) {}

  // Returns the Cost of the best solution across all threads.
  SavedAssignment trySolution(Int threadId, const SavedAssignment& solution);

  [[nodiscard]] Int getBestThreadId() const { return _bestThread; }

  [[nodiscard]] Cost getCost() const { return _bestCost.value(); }

  [[nodiscard]] SavedAssignment getSolution() const {
    return _solution.value();
  }

  [[nodiscard]] bool shouldPrint(Int threadId) const;

  void hasPrinted() const;
};

}  // namespace atlantis::search

#pragma once
#include <mutex>
#include <optional>

#include "cost.hpp"

namespace atlantis::search {

class ThreadController {
  bool _hasSolution;
  std::optional<Cost> _bestCost;
  Int _bestThread;
  mutable std::mutex _lock;
  Int _counter;

  void lock(Int threadId) const;
  void unlock(Int threadId) const;

 public:
  ThreadController();

  // Returns the Cost of the best solution across all threads.
  Cost trySolution(Int threadId, Cost cost);

  [[nodiscard]] Int getBestThreadId() const { return _bestThread; }

  [[nodiscard]] Cost getCost() const { return _bestCost.value(); }
};

}  // namespace atlantis::search

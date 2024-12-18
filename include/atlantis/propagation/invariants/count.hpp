#pragma once

#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

/**
 * output <- number of occurrences of needle in variables
 *
 */

class Count : public Invariant {
  VarId _output;
  VarViewId _needle;
  std::vector<VarViewId> _vars;
  std::vector<CommittableInt> _counts;
  Int _offset;
  void increaseCount(Timestamp ts, Int value);
  void decreaseCount(Timestamp ts, Int value);
  [[nodiscard]] signed char count(Timestamp ts, Int value) const;

 public:
  explicit Count(SolverBase&, VarId output, VarViewId needle,
                 std::vector<VarViewId>&& varArray);

  explicit Count(SolverBase&, VarViewId output, VarViewId needle,
                 std::vector<VarViewId>&& varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation

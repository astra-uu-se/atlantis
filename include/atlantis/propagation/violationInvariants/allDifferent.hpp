#pragma once

#include <optional>

#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"

namespace atlantis::propagation {

class AllDifferent : public ViolationInvariant {
 protected:
  std::vector<VarViewId> _vars;
  std::vector<CommittableInt> _counts;
  Int _offset;
  [[nodiscard]] std::optional<size_t> countIndex(Int value) const;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  explicit AllDifferent(SolverBase&, VarId violationId,
                        std::vector<VarViewId>&& vars);

  explicit AllDifferent(SolverBase&, VarViewId violationId,
                        std::vector<VarViewId>&& vars);

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

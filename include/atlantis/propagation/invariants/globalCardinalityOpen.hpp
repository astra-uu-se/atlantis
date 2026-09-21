#pragma once

#include <optional>

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

class GlobalCardinalityOpen : public Invariant {
  std::vector<VarId> _outputs;
  std::vector<VarViewId> _inputs;
  std::vector<Int> _cover;
  std::vector<Int> _coverVarIndex;
  std::vector<CommittableInt> _counts;
  Int _offset;
  [[nodiscard]] std::optional<size_t> coverIndex(Int value) const;
  void increaseCount(Timestamp ts, Int value);
  void decreaseCountAndUpdateOutput(Timestamp ts, Int value);
  void increaseCountAndUpdateOutput(Timestamp ts, Int value);

 public:
  GlobalCardinalityOpen(SolverBase&, std::vector<VarId>&& outputs,
                        std::vector<VarViewId>&& inputs,
                        std::vector<Int>&& cover);

  GlobalCardinalityOpen(SolverBase&, std::vector<VarViewId>&& outputs,
                        std::vector<VarViewId>&& inputs,
                        std::vector<Int>&& cover);

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

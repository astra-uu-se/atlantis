#pragma once

#include <vector>

#include "atlantis/propagation/violationInvariants/allDifferent.hpp"

namespace atlantis::propagation {

class AllDifferentExcept : public AllDifferent {
  std::vector<bool> _ignored;
  Int _ignoredOffset;

  [[nodiscard]] bool isIgnored(Int) const;

 public:
  explicit AllDifferentExcept(SolverBase&, VarId violationId,
                              std::vector<VarViewId>&& vars,
                              const std::vector<Int>& ignored);

  explicit AllDifferentExcept(SolverBase&, VarViewId violationId,
                              std::vector<VarViewId>&& vars,
                              const std::vector<Int>& ignored);

  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
};

}  // namespace atlantis::propagation

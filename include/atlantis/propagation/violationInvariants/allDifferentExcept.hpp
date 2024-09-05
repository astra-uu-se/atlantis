#pragma once

#include <vector>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class AllDifferentExcept : public AllDifferent {
 private:
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

inline bool AllDifferentExcept::isIgnored(const Int val) const {
  return _ignoredOffset <= val &&
         static_cast<size_t>(val - _ignoredOffset) < _ignored.size() &&
         _ignored[val - _ignoredOffset];
}

}  // namespace atlantis::propagation

#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/propagation/variables/intVar.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class AllDifferentExcept : public AllDifferent {
 private:
  std::vector<bool> _ignored;
  Int _ignoredOffset;

  [[nodiscard]] bool isIgnored(Int) const;

 public:
  explicit AllDifferentExcept(SolverBase&, VarId violationId,
                              std::vector<VarId>&& vars,
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

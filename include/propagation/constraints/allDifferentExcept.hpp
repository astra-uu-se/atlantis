#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "constraint.hpp"
#include "types.hpp"
#include "propagation/constraints/allDifferent.hpp"
#include "propagation/engine.hpp"
#include "propagation/variables/committableInt.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::propagation {

class AllDifferentExcept : public AllDifferent {
 private:
  std::vector<bool> _ignored;
  Int _ignoredOffset;

  bool isIgnored(Int) const;

 public:
  explicit AllDifferentExcept(Engine&, VarId violationId,
                              std::vector<VarId> variables,
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
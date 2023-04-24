#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "constraint.hpp"
#include "constraints/allDifferent.hpp"
#include "core/engine.hpp"
#include "core/types.hpp"
#include "variables/committableInt.hpp"
#include "variables/intVar.hpp"

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
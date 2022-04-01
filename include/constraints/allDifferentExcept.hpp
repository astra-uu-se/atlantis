#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "constraint.hpp"
#include "constraints/allDifferent.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class CommittableInt;  // forward declare
class Engine;

class AllDifferentExcept : public AllDifferent {
 private:
  std::vector<bool> _ignored;
  Int _ignoredOffset;

  bool isIgnored(Int) const;

 public:
  AllDifferentExcept(VarId violationId, std::vector<VarId> variables,
                     const std::vector<Int>& ignored);

  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
};

inline bool AllDifferentExcept::isIgnored(const Int val) const {
  return _ignoredOffset <= val &&
         static_cast<size_t>(val - _ignoredOffset) < _ignored.size() &&
         _ignored[val - _ignoredOffset];
}
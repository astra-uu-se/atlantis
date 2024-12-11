#pragma once

#include <cassert>
#include <cstdint>

namespace atlantis {

using UInt64 = std::uint64_t;
using UInt = UInt64;
using Int = int64_t;
using Timestamp = UInt;
[[maybe_unused]] static Timestamp NULL_TIMESTAMP = Timestamp(0);

enum class ObjectiveDirection : char { MINIMIZE = 1, MAXIMIZE = -1, NONE = 0 };

struct DomainEntry {
  Int lowerBound;
  Int upperBound;
  DomainEntry(Int lb, Int ub) : lowerBound(lb), upperBound(ub) {
    assert(lb <= ub);
  }
};

}  // namespace atlantis
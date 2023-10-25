#pragma once

#include <cassert>
#include <cstdint>

namespace atlantis {

using UInt64 = std::uint64_t;
using UInt32 = std::uint32_t;
using UInt = UInt64;
using Int = int64_t;

struct DomainEntry {
  Int lowerBound;
  Int upperBound;
  DomainEntry(Int lb, Int ub) : lowerBound(lb), upperBound(ub) {
    assert(lb <= ub);
  }
};

}  // namespace atlantis
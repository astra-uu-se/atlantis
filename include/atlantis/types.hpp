#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace atlantis {

using Int = int64_t;
using UInt = std::make_unsigned_t<Int>;
using Timestamp = UInt;
[[maybe_unused]] static Timestamp NULL_TIMESTAMP = Timestamp{0};

enum class ObjectiveDirection : char { MINIMIZE = 1, MAXIMIZE = -1, NONE = 0 };

struct DomainEntry {
  Int lowerBound;
  Int upperBound;
  DomainEntry(const Int lb, const Int ub) : lowerBound(lb), upperBound(ub) {
    assert(lb <= ub);
  }
};

}  // namespace atlantis
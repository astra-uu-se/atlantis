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

enum class RelationType : signed char {
  REL_TYPE_EQ, // Equality
  REL_TYPE_NE, // Disequality
  REL_TYPE_LE, // Less or equal
  REL_TYPE_LT, // Strictly less
  REL_TYPE_GE, // Greater or equal
  REL_TYPE_GT  // Strictly greater
};

inline RelationType invertRelationType(const RelationType relationType) {
  switch (relationType) {
    case RelationType::REL_TYPE_EQ: return RelationType::REL_TYPE_NE;
    case RelationType::REL_TYPE_NE: return RelationType::REL_TYPE_EQ;
    case RelationType::REL_TYPE_GE: return RelationType::REL_TYPE_LT;
    case RelationType::REL_TYPE_LE: return RelationType::REL_TYPE_GT;
    case RelationType::REL_TYPE_LT: return RelationType::REL_TYPE_GE;
    case RelationType::REL_TYPE_GT: return RelationType::REL_TYPE_LE;
    default: return RelationType::REL_TYPE_EQ;
  }
}

}  // namespace atlantis
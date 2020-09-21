#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>
using UInt64 = u_int64_t;
using UInt32 = u_int32_t;
using UInt = UInt64;
using Int = int64_t;
using Timestamp = u_int64_t;
using IdBase = size_t;  // IDs are mainly used for vector lookups, so they must
                        // be of size_t.

[[maybe_unused]] static Timestamp NULL_TIMESTAMP = Timestamp(0);

struct Id {
  IdBase id;
  Id() = delete;
  Id(size_t i) : id(i) {}
  operator size_t() const { return id; }
  // TODO: We should just overload the == operator but I am too scared to do it.
  inline bool equals(const Id& other) { return id == other.id; }
};

static Id NULL_ID = Id(0);

struct VarId : public Id {
  VarId(size_t i) : Id(i) {}
  VarId(Id& t_id) : Id(t_id.id) {}
};
struct LocalId : public Id {
  LocalId(size_t i) : Id(i) {}
  LocalId(Id& t_id) : Id(t_id.id) {}
  LocalId(VarId& t_id) : Id(t_id.id) {}
};
struct InvariantId : public Id {
  InvariantId(size_t i) : Id(i) {}
  InvariantId(Id& t_id) : Id(t_id.id) {}
};
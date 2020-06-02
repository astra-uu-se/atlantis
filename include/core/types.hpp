#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>
using UInt64 = u_int64_t;
using UInt32 = u_int32_t;
using UInt = UInt64;
using Int = int64_t;
using Timestamp = u_int64_t;
using IdBase = u_int64_t;

struct Id {
  IdBase id;
  Id(int i) : id(i) {}
  operator int() const { return id; }
};

struct VarId : public Id {
  VarId(Id& t_id) : Id(t_id.id) {}
};
struct LocalId : public Id {
  LocalId(int i) : Id(i) {}
  LocalId(Id& t_id) : Id(t_id.id) {}
};
struct InvariantId : public Id {
  InvariantId(Id& t_id) : Id(t_id.id) {}
};
#pragma once

#include <cstddef>
#include <cstdint>
using UInt64 = std::uint64_t;
using UInt32 = std::uint32_t;
using UInt = UInt64;
using Int = int64_t;
using Timestamp = UInt;
using IdBase = size_t;  // IDs are mainly used for vector lookups, so they must
                        // be of size_t.
enum VarIdType { var, view };

[[maybe_unused]] static Timestamp NULL_TIMESTAMP = Timestamp(0);

struct Id {
  IdBase id;
  Id() : id(0) {}
  explicit Id(size_t i) : id(i) {}
  operator size_t() const { return id; }
  inline bool equals(const Id& other) const { return id == other.id; }
};

static Id NULL_ID = Id();

struct InvariantId;  // forward declare
struct VarIdBase : public Id {
  VarIdBase() : Id() {}
  VarIdBase(size_t i) : Id(i) {}
  VarIdBase(const Id& t_id) : Id(t_id.id) {}
  VarIdBase(const InvariantId&) = delete;
};
struct VarId : public VarIdBase {
  VarIdType idType;
  VarId() : VarIdBase(), idType(VarIdType::var) {}
  VarId(size_t i, VarIdType type) : VarIdBase(i), idType(type) {}
  VarId(size_t i) : VarId(i, VarIdType::var) {}
  VarId(const Id& t_id, VarIdType type) : VarIdBase(t_id.id), idType(type) {}
  VarId(const Id& t_id) : VarId(t_id, VarIdType::var) {}
  VarId(const InvariantId&) = delete;
};
struct LocalId : public Id {
  LocalId() : Id() {}
  LocalId(size_t i) : Id(i) {}
  LocalId(const Id& t_id) : Id(t_id.id) {}
  LocalId(const VarId& t_id) : Id(t_id.id) {}
  LocalId(const InvariantId&) = delete;
};
struct InvariantId : public Id {
  InvariantId() : Id() {}
  InvariantId(size_t i) : Id(i) {}
  InvariantId(const Id& t_id) : Id(t_id.id) {}
  InvariantId(const VarIdBase&) = delete;
  InvariantId(const VarId&) = delete;
  InvariantId(const LocalId&) = delete;
};

enum class PropagationMode { INPUT_TO_OUTPUT, OUTPUT_TO_INPUT };
enum class OutputToInputMarkingMode {
  // No marking:
  NONE,
  // Mark dfs starting from modified search variables:
  INPUT_TO_OUTPUT_EXPLORATION,
  // Statically (when closing the model), for each var x, store the set of
  // search variables x (transitively) depends on.
  // When propagating a var x, x is marked if the set of modified search
  // variables and the set of search variables x depends on overlaps:
  OUTPUT_TO_INPUT_STATIC
};
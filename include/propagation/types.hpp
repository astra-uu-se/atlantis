#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "../types.hpp"

namespace atlantis::propagation {

using Timestamp = UInt;
using IdBase = size_t;  // IDs are mainly used for vector lookups, so they must
                        // be of size_t.
enum class VarIdType : bool { var, view };

[[maybe_unused]] static Timestamp NULL_TIMESTAMP = Timestamp(0);

struct Id {
  IdBase id;
  Id() : id(0) {}
  virtual ~Id() = default;
  explicit Id(size_t i) : id(i) {}
  operator size_t() const { return id; }
  [[nodiscard]] inline bool operator==(const Id& other) const {
    return id == other.id;
  }
  [[nodiscard]] inline bool operator!=(const Id& other) const {
    return !operator==(other);
  }
};

static Id NULL_ID = Id();

struct InvariantId;  // forward declare
struct VarId;
struct VarIdBase : public Id {
  VarIdBase() : Id() {}
  VarIdBase(size_t i) : Id(i) {}
  VarIdBase(const Id& t_id) : Id(t_id.id) {}
  VarIdBase(const InvariantId&) = delete;
  [[nodiscard]] inline bool operator==(const IdBase& other) const {
    return id == other;
  }
  [[nodiscard]] inline bool operator==(const Id& other) const {
    return id == other.id;
  }
  [[nodiscard]] inline bool operator==(const VarIdBase& other) const {
    return id == other.id;
  }
  [[nodiscard]] bool operator!=(const IdBase& other) const {
    return !operator==(other);
  }
  [[nodiscard]] bool operator!=(const Id& other) const {
    return !operator==(other);
  }
  [[nodiscard]] bool operator!=(const VarIdBase& other) const {
    return !operator==(other);
  }
};
struct VarId : public VarIdBase {
  VarIdType idType;
  VarId() : VarIdBase(), idType(VarIdType::var) {}
  VarId(size_t i, VarIdType type) : VarIdBase(i), idType(type) {}
  VarId(size_t i) : VarId(i, VarIdType::var) {}
  VarId(const Id& t_id, VarIdType type) : VarIdBase(t_id.id), idType(type) {}
  VarId(const Id& t_id) : VarId(t_id, VarIdType::var) {}
  VarId(const InvariantId&) = delete;
  [[nodiscard]] inline bool operator==(const IdBase& other) const {
    return id == other;
  }
  [[nodiscard]] inline bool operator==(const Id& other) const {
    return id == other.id;
  }
  [[nodiscard]] inline bool operator==(const VarIdBase& other) const {
    return id == other.id;
  }
  [[nodiscard]] inline bool operator==(const VarId& other) const {
    return idType == other.idType && id == other.id;
  }
  [[nodiscard]] inline bool operator!=(const IdBase& other) const {
    return !operator==(other);
  }
  [[nodiscard]] inline bool operator!=(const Id& other) const {
    return !operator==(other);
  }
  [[nodiscard]] inline bool operator!=(const VarIdBase& other) const {
    return !operator==(other);
  }
  [[nodiscard]] inline bool operator!=(const VarId& other) const {
    return !operator==(other);
  }
};
struct LocalId : public Id {
  LocalId() : Id() {}
  LocalId(size_t i) : Id(i) {}
  LocalId(const Id& t_id) : Id(t_id.id) {}
  LocalId(const VarId& t_id) : Id(t_id.id) {}
  LocalId(const InvariantId&) = delete;
  [[nodiscard]] virtual bool operator==(const IdBase& other) const {
    return id == other;
  }
  [[nodiscard]] virtual bool operator==(const Id& other) const {
    return id == other.id;
  }
  [[nodiscard]] virtual bool operator==(const LocalId& other) const {
    return id == other.id;
  }
  [[nodiscard]] virtual bool operator!=(const IdBase& other) const {
    return !operator==(other);
  }
  [[nodiscard]] virtual bool operator!=(const Id& other) const {
    return !operator==(other);
  }
  [[nodiscard]] virtual bool operator!=(const LocalId& other) const {
    return !operator==(other);
  }
};
struct InvariantId : public Id {
  InvariantId() : Id() {}
  InvariantId(size_t i) : Id(i) {}
  InvariantId(const Id& t_id) : Id(t_id.id) {}
  InvariantId(const VarIdBase&) = delete;
  InvariantId(const VarId&) = delete;
  InvariantId(const LocalId&) = delete;
  [[nodiscard]] bool operator==(const IdBase& other) const {
    return id == other;
  }
  [[nodiscard]] bool operator==(const Id& other) const {
    return id == other.id;
  }
  [[nodiscard]] bool operator==(const InvariantId& other) const {
    return id == other.id;
  }
  [[nodiscard]] bool operator!=(const IdBase& other) const {
    return !operator==(other);
  }
  [[nodiscard]] bool operator!=(const Id& other) const {
    return !operator==(other);
  }
  [[nodiscard]] bool operator!=(const InvariantId& other) const {
    return !operator==(other);
  }
};

enum class CommitMode : bool { NO_COMMIT, COMMIT };
enum class PropagationMode : bool { INPUT_TO_OUTPUT, OUTPUT_TO_INPUT };
enum class OutputToInputMarkingMode : char {
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

// Required because VarId implicitly converts to size_t, thereby ignoring the
// idType field. This means VarIds cannot be used as keys in a hashing
// container if views and intvars are mixed.
struct VarIdHash {
  std::size_t operator()(VarId const& varId) const noexcept {
    std::size_t typeHash = std::hash<int>{}(static_cast<int>(varId.idType));
    return typeHash ^ (varId.id << 1);
  }
};

enum class ObjectiveDirection : char { MINIMIZE = 1, MAXIMIZE = -1, NONE = 0 };

}  // namespace atlantis::propagation
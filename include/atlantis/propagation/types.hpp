#pragma once

#include <climits>
#include <cstddef>

#include "atlantis/types.hpp"

namespace atlantis::propagation {

using VarId = size_t;
using ViewId = size_t;
using InvariantId = size_t;
using LocalId = size_t;
[[maybe_unused]] static size_t NULL_ID = ~size_t{0};

struct VarViewId {
 private:
  size_t id;

 public:
  static constexpr size_t VIEW_MASK =
      (size_t{1} << (sizeof(size_t) * CHAR_BIT - 1));

  VarViewId(size_t i) : id(i == NULL_ID ? i : (i & ~VIEW_MASK)) {}

  VarViewId(size_t i, bool isView)
      : id(i == NULL_ID ? i : (isView ? (i | VIEW_MASK) : (i & ~VIEW_MASK))) {}

  [[nodiscard]] bool isView() const {
    return id != NULL_ID && (id & VIEW_MASK) != size_t{0};
  }

  [[nodiscard]] bool isVar() const {
    return id != NULL_ID && (id & VIEW_MASK) == size_t{0};
  }

  [[nodiscard]] bool operator==(size_t other) const {
    return size_t(id) == other;
  }

  [[nodiscard]] bool operator==(const VarViewId& other) const {
    return id == other.id;
  }

  [[nodiscard]] bool operator!=(size_t other) const {
    return !operator==(other);
  }

  [[nodiscard]] bool operator!=(const VarViewId& other) const {
    return !operator==(other);
  }

  explicit operator size_t() const {
    return id == NULL_ID ? id : (id & ~VIEW_MASK);
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

}  // namespace atlantis::propagation
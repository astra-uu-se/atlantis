#pragma once

#include <climits>
#include <ostream>
#include <string>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

static size_t NULL_NODE_ID = ~size_t{0};

using VarNodeId = size_t;

struct InvariantNodeId {
 private:
  size_t _id;
  static const size_t IMPLICIT_CONSTRAINT_MASK =
      (size_t{1} << (sizeof(size_t) * CHAR_BIT - 1));

 public:
  InvariantNodeId(const InvariantNodeId&) = default;

  InvariantNodeId(size_t id, bool isImplicitConstraint)
      : _id(id == NULL_NODE_ID
                ? id
                : (isImplicitConstraint ? (id | IMPLICIT_CONSTRAINT_MASK)
                                        : (id & ~IMPLICIT_CONSTRAINT_MASK))) {}

  explicit InvariantNodeId(size_t id) : InvariantNodeId(id, false) {}

  [[nodiscard]] bool isImplicitConstraint() const {
    return _id != NULL_NODE_ID && (_id & IMPLICIT_CONSTRAINT_MASK) != size_t{0};
  }

  [[nodiscard]] bool isInvariant() const {
    return _id != NULL_NODE_ID && (_id & IMPLICIT_CONSTRAINT_MASK) == size_t{0};
  }

  InvariantNodeId& operator=(const InvariantNodeId& other) {
    _id = other._id;
    return *this;
  }

  bool operator==(const InvariantNodeId& other) const {
    return _id == other._id;
  }

  bool operator==(size_t other) const { return other == size_t(_id); }

  bool operator!=(const InvariantNodeId& other) const {
    return !(operator==(other));
  }
  friend std::ostream& operator<<(std::ostream& os,
                                  const InvariantNodeId& invariantNodeId) {
    return os << (invariantNodeId.isImplicitConstraint() ? "impl_" : "inv_")
              << size_t(invariantNodeId);
  }

  bool operator!=(size_t other) const { return !(operator==(other)); }

  explicit operator size_t() const {
    return _id == NULL_NODE_ID ? _id : (_id & ~IMPLICIT_CONSTRAINT_MASK);
  }
};

struct InvariantNodeIdHash {
  std::size_t operator()(
      InvariantNodeId const& invariantNodeId) const noexcept {
    return size_t(invariantNodeId);
  }
};

struct InvariantGraphOutputVarArray {
  std::string identifier;
  std::vector<Int> indexSetSizes;
  std::vector<VarNodeId> varNodeIds;

  InvariantGraphOutputVarArray(const std::string& identifier,
                               const std::vector<Int>& indexSetSizes,
                               const std::vector<VarNodeId>& varNodeIds)
      : identifier(identifier),
        indexSetSizes(indexSetSizes),
        varNodeIds(varNodeIds) {}
};

enum struct InvariantNodeState : unsigned char {
  UNINITIALIZED,
  ACTIVE,
  SUBSUMED
};

struct InvariantGraphEdge {
  InvariantNodeId invariantNodeId;
  VarNodeId varNodeId;
};

enum struct DomainType : unsigned char {
  DOM_NONE = 0,
  DOM_FIXED = 1,
  DOM_LOWER_BOUND = 2,
  DOM_UPPER_BOUND = 3,
  DOM_RANGE = 4,
  DOM_DOMAIN = 5
};

}  // namespace atlantis::invariantgraph
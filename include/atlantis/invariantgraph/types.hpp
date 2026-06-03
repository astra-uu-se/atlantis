#pragma once

#include <climits>
#include <ostream>
#include <string>
#include <utility>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

static size_t NULL_NODE_ID = ~size_t{0};

using VarNodeId = size_t;

struct InvariantNodeId {
  friend struct InvariantNodeIdHash;

 private:
  size_t _id;
  static constexpr size_t IMPLICIT_CONSTRAINT_MASK =
      (size_t{1} << (sizeof(size_t) * CHAR_BIT - 1));

 public:
  InvariantNodeId(const InvariantNodeId&) = default;

  InvariantNodeId(const size_t id, const bool isImplicitConstraint)
      : _id(id == NULL_NODE_ID
                ? id
                : (isImplicitConstraint ? (id | IMPLICIT_CONSTRAINT_MASK)
                                        : (id & ~IMPLICIT_CONSTRAINT_MASK))) {}

  explicit InvariantNodeId(const size_t id) : InvariantNodeId(id, false) {}

  [[nodiscard]] bool isImplicitConstraint() const {
    return _id != NULL_NODE_ID && (_id & IMPLICIT_CONSTRAINT_MASK) != size_t{0};
  }

  [[nodiscard]] bool isInvariant() const {
    return _id != NULL_NODE_ID && (_id & IMPLICIT_CONSTRAINT_MASK) == size_t{0};
  }

  InvariantNodeId& operator=(const InvariantNodeId& other) = default;

  bool operator==(const InvariantNodeId& other) const {
    return _id == other._id;
  }

  bool operator==(const size_t other) const { return _id == other; }

  bool operator!=(const InvariantNodeId& other) const {
    return _id != other._id;
  }

  bool operator!=(const size_t other) const { return _id != other; }

  friend std::ostream& operator<<(std::ostream& os,
                                  const InvariantNodeId& invariantNodeId) {
    return os << (invariantNodeId.isImplicitConstraint() ? "impl_" : "inv_")
              << size_t{invariantNodeId};
  }

  explicit operator size_t() const {
    return _id == NULL_NODE_ID ? _id : (_id & ~IMPLICIT_CONSTRAINT_MASK);
  }
};

struct InvariantNodeIdHash {
  std::size_t operator()(
      InvariantNodeId const& invariantNodeId) const noexcept {
    return invariantNodeId._id;
  }
};

struct ConstraintVarId {
 private:
  size_t _id;
  static constexpr size_t BOOL_VAR_MASK =
      (size_t{1} << (sizeof(size_t) * CHAR_BIT - 1));

 public:
  ConstraintVarId(const ConstraintVarId&) = default;

  ConstraintVarId(const size_t id, const bool isIntVar)
      : _id(id == NULL_NODE_ID
                ? id
                : (isIntVar ? (id & ~BOOL_VAR_MASK) : (id | BOOL_VAR_MASK))) {}

  explicit ConstraintVarId(const size_t id) : ConstraintVarId(id, true) {}

  [[nodiscard]] bool isBoolVar() const {
    return _id != NULL_NODE_ID && (_id & BOOL_VAR_MASK) != size_t{0};
  }

  [[nodiscard]] bool isIntVar() const {
    return _id != NULL_NODE_ID && (_id & BOOL_VAR_MASK) == size_t{0};
  }

  ConstraintVarId& operator=(const ConstraintVarId& other) = default;

  bool operator==(const ConstraintVarId& other) const {
    return _id == other._id;
  }

  bool operator==(const size_t other) const { return other == size_t{_id}; }

  bool operator!=(const ConstraintVarId& other) const {
    return _id != other._id;
  }

  bool operator!=(const size_t other) const { return _id != other; }

  friend std::ostream& operator<<(std::ostream& os,
                                  const ConstraintVarId& invariantNodeId) {
    return os << (invariantNodeId.isBoolVar() ? "bool_" : "int_")
              << size_t{invariantNodeId};
  }

  explicit operator size_t() const {
    return _id == NULL_NODE_ID ? _id : (_id & ~BOOL_VAR_MASK);
  }
};

struct InvariantGraphOutputVarArray {
  std::string identifier;
  std::vector<Int> indexSetSizes{};
  std::vector<VarNodeId> varNodeIds{};

  explicit InvariantGraphOutputVarArray(
      std::string&& m_identifier, const std::vector<Int>& m_indexSetSizes,
      const std::vector<VarNodeId>& m_varNodeIds)
      : identifier(std::move(m_identifier)),
        indexSetSizes(m_indexSetSizes),
        varNodeIds(m_varNodeIds) {}

  explicit InvariantGraphOutputVarArray(
      const std::string& m_identifier, const std::vector<Int>& m_indexSetSizes,
      const std::vector<VarNodeId>& m_varNodeIds)
      : InvariantGraphOutputVarArray(std::string(m_identifier), m_indexSetSizes,
                                     m_varNodeIds) {}
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

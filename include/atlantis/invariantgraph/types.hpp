#pragma once

#include <limits.h>

#include <cstdint>
#include <string>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

struct NodeId {
 protected:
  size_t id;

 public:
  NodeId() : id(0) {}

  explicit NodeId(size_t i) : id(i) {}

  NodeId(const NodeId&) = default;

  explicit operator size_t() const { return id; }
};

static NodeId NULL_NODE_ID = NodeId(~size_t{0});

struct VarNodeId : public NodeId {
  VarNodeId() : NodeId() {}
  VarNodeId(const VarNodeId&) = default;
  explicit VarNodeId(size_t i) : NodeId(i) {}
  explicit VarNodeId(NodeId nodeId) : NodeId(nodeId) {}

  inline VarNodeId& operator=(const VarNodeId& other) {
    id = size_t(other);
    return *this;
  }
  inline VarNodeId& operator=(const NodeId& other) {
    id = size_t(other);
    return *this;
  }

  inline bool operator==(const VarNodeId& other) const {
    return id == size_t(other);
  }

  inline bool operator==(const NodeId& other) const {
    return id == size_t(other);
  }

  explicit operator size_t() const { return size_t(id); }

  bool operator!=(const VarNodeId& other) const { return !(operator==(other)); }
  bool operator!=(const NodeId& other) const { return !(operator==(other)); }
};

struct InvariantNodeId : public NodeId {
 private:
  static const size_t IMPLICIT_CONSTRAINT_MASK =
      (size_t{1} << (sizeof(size_t) * CHAR_BIT - 1));

 public:
  InvariantNodeId(const InvariantNodeId&) = default;

  InvariantNodeId() : NodeId() {}

  InvariantNodeId(NodeId nodeId) : NodeId(size_t(nodeId)) {}

  explicit InvariantNodeId(size_t i)
      : NodeId(i == size_t(NULL_NODE_ID) ? i
                                         : (i & ~IMPLICIT_CONSTRAINT_MASK)) {}

  InvariantNodeId(size_t i, bool isImplicitConstraint)
      : NodeId(i == size_t(NULL_NODE_ID)
                   ? i
                   : (isImplicitConstraint ? (i | IMPLICIT_CONSTRAINT_MASK)
                                           : (i & ~IMPLICIT_CONSTRAINT_MASK))) {
  }

  inline bool isImplicitConstraint() const {
    return id != size_t(NULL_NODE_ID) &&
           (id & IMPLICIT_CONSTRAINT_MASK) != size_t{0};
  }

  inline bool isInvariant() const {
    return id != size_t(NULL_NODE_ID) &&
           (id & IMPLICIT_CONSTRAINT_MASK) == size_t{0};
  }

  inline InvariantNodeId& operator=(const InvariantNodeId& other) {
    id = other.id;
    return *this;
  }

  bool operator==(const InvariantNodeId& other) const { return id == other.id; }

  bool operator!=(const InvariantNodeId& other) const {
    return !(operator==(other));
  }

  explicit operator size_t() const {
    return id == size_t(NULL_NODE_ID) ? id : (id & ~IMPLICIT_CONSTRAINT_MASK);
  }
};

struct VarNodeIdHash {
  std::size_t operator()(VarNodeId const& varNodeId) const noexcept {
    return size_t(varNodeId);
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
  std::vector<invariantgraph::VarNodeId> varNodeIds;

  InvariantGraphOutputVarArray(
      std::string identifier, std::vector<Int> indexSetSizes,
      std::vector<invariantgraph::VarNodeId> varNodeIds)
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

}  // namespace atlantis::invariantgraph
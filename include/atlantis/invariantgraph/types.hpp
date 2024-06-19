#pragma once

#include <cstdint>
#include <string>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

struct NodeId {
  size_t id;
  NodeId() : id(0) {}
  explicit NodeId(size_t i) : id(i) {}
  NodeId(const NodeId&) = default;
};

static NodeId NULL_NODE_ID = NodeId();

struct VarNodeId : public NodeId {
  VarNodeId() : NodeId() {}
  VarNodeId(const VarNodeId&) = default;
  explicit VarNodeId(size_t i) : NodeId(i) {}
  explicit VarNodeId(NodeId nodeId) : NodeId(nodeId) {}

  inline VarNodeId& operator=(const VarNodeId& other) {
    id = other.id;
    return *this;
  }
  inline VarNodeId& operator=(const NodeId& other) {
    id = other.id;
    return *this;
  }

  inline bool operator==(const VarNodeId& other) const {
    return id == other.id;
  }

  inline bool operator==(const NodeId& other) const {
    return other.id == NULL_NODE_ID.id && NULL_NODE_ID.id == id;
  }

  bool operator!=(const VarNodeId& other) const { return !(operator==(other)); }
  bool operator!=(const NodeId& other) const { return !(operator==(other)); }
};

struct InvariantNodeId : public NodeId {
  enum struct Type : bool { INVARIANT = false, IMPLICIT_CONSTRAINT = true };
  Type type;

  InvariantNodeId(const InvariantNodeId&) = default;

  InvariantNodeId() : NodeId(), type(Type::INVARIANT) {}
  explicit InvariantNodeId(size_t i) : NodeId(i), type(Type::INVARIANT) {}
  InvariantNodeId(size_t i, bool isImplicitConstraint)
      : NodeId(i),
        type(isImplicitConstraint ? Type::IMPLICIT_CONSTRAINT
                                  : Type::INVARIANT) {}
  explicit InvariantNodeId(NodeId nodeId)
      : NodeId(nodeId), type(Type::INVARIANT) {}

  inline InvariantNodeId& operator=(const InvariantNodeId& other) {
    id = other.id;
    type = other.type;
    return *this;
  }
  inline InvariantNodeId& operator=(const NodeId& other) {
    id = other.id;
    return *this;
  }

  bool operator==(const InvariantNodeId& other) const {
    return type == other.type && id == other.id;
  }
  inline bool operator==(const NodeId& other) const {
    return other.id == NULL_NODE_ID.id && NULL_NODE_ID.id == id;
  }

  bool operator!=(const InvariantNodeId& other) const {
    return !(operator==(other));
  }
  bool operator!=(const NodeId& other) const { return !(operator==(other)); }
};

struct VarNodeIdHash {
  std::size_t operator()(VarNodeId const& varNodeId) const noexcept {
    return varNodeId.id;
  }
};

struct InvariantNodeIdHash {
  std::size_t operator()(
      InvariantNodeId const& invariantNodeId) const noexcept {
    std::size_t typeHash =
        std::hash<int>{}(static_cast<int>(invariantNodeId.type));
    return typeHash ^ (invariantNodeId.id << 1);
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

}  // namespace atlantis::invariantgraph
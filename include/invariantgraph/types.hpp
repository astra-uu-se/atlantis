#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace invariantgraph {

struct NodeId {
  size_t id;
  NodeId() : id(0) {}
  NodeId(size_t i) : id(i) {}
  NodeId(const NodeId&) = default;
};

static NodeId NULL_NODE_ID = NodeId();

struct VarNodeId : public NodeId {
  VarNodeId() : NodeId() {}
  VarNodeId(const VarNodeId&) = default;
  VarNodeId(size_t i) : NodeId(i) {}
  VarNodeId(const NodeId& nodeId) : NodeId(nodeId) {}

  inline bool operator==(const VarNodeId& other) const {
    return id == other.id;
  }
  inline bool operator==(const NodeId& other) const { return id == other.id; }

  bool operator!=(const VarNodeId& other) const { return !(operator==(other)); }
  bool operator!=(const NodeId& other) const { return !(operator==(other)); }
};

struct InvariantNodeId : public NodeId {
  enum struct Type : bool { INVARIANT = false, IMPLICIT_CONSTRAINT = true };
  Type type;

  InvariantNodeId(const InvariantNodeId&) = default;

  InvariantNodeId() : NodeId(), type(Type::INVARIANT) {}
  InvariantNodeId(size_t i) : NodeId(i), type(Type::INVARIANT) {}
  InvariantNodeId(size_t i, bool isImplicitConstraint)
      : NodeId(i),
        type(isImplicitConstraint ? Type::IMPLICIT_CONSTRAINT
                                  : Type::INVARIANT) {}
  InvariantNodeId(NodeId nodeId) : NodeId(nodeId), type(Type::INVARIANT) {}

  bool operator==(const InvariantNodeId& other) const {
    return type == other.type && id == other.id;
  }
  bool operator!=(const InvariantNodeId& other) const {
    return !(operator==(other));
  }
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

}  // namespace invariantgraph
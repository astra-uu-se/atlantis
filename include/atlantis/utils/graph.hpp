#pragma once
#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis {

class NodeBase {
  size_t _id;
public:
  explicit NodeBase(size_t);
  virtual ~NodeBase() = default;
  size_t id() const noexcept;
  bool isSource() const noexcept;
  bool isSink() const noexcept;
  virtual const std::vector<size_t>& outgoing() const = 0;
  virtual const std::vector<size_t>& incoming() const = 0;
};
inline NodeBase::NodeBase(const size_t i) { _id = i; }
inline bool NodeBase::isSource() const noexcept { return incoming().empty(); }
inline bool NodeBase::isSink() const noexcept { return outgoing().empty(); }

template <class Node, std::enable_if_t<std::is_base_of_v<Node, NodeBase>, bool> = true>
class Graph {
  std::vector<Node> _nodes;

  void scc_util(size_t nodeId, std::vector<Int>& discoverTime, std::vector<Int>& lowTime,
                    std::vector<size_t>& stack, std::vector<bool>& onStack,
                    Int& time,
                    std::vector<std::vector<size_t>>& components);
public:
  explicit Graph(size_t numNodes);

  void addNode(Node&& node);

  const Node& node(size_t i) const;

  std::vector<size_t> scc();

  std::vector<size_t> findCycle(const std::vector<size_t>& component);

  std::pair<size_t, size_t> findPivot(std::vector<size_t>& cycle);

};
template <class Node,
          std::enable_if_t<std::is_base_of_v<Node, NodeBase>, bool> E0>
void Graph<Node, E0>::scc_util(size_t nodeId, std::vector<Int>& discoverTime,
                               std::vector<Int>& lowTime,
                               std::vector<size_t>& stack,
                               std::vector<bool>& onStack, Int& time,
                               std::vector<std::vector<size_t>>& components) {}
template <class Node,
          std::enable_if_t<std::is_base_of_v<Node, NodeBase>, bool> E0>
Graph<Node, E0>::Graph(const size_t numNodes) { _nodes.reserve(numNodes); }
template <class Node,
          std::enable_if_t<std::is_base_of_v<Node, NodeBase>, bool> E0>
void Graph<Node, E0>::addNode(Node&& node) { _nodes.emplace_back(std::move(node)); }
template <class Node,
          std::enable_if_t<std::is_base_of_v<Node, NodeBase>, bool> E0>
const Node& Graph<Node, E0>::node(const size_t i) const { assert(i < _nodes.size()); return _nodes[i]; }
}  // namespace atlantis
#pragma once
#include <limits>

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis {


class Graph {
public:
  class Node {
  public:
    struct Iterator {
      using iterator_category = std::forward_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = size_t;
      using pointer = size_t const*;
      using reference = const size_t&;

    private:
      const std::vector<size_t>& _first;
      const std::vector<size_t>& _second;
      size_t _index;

    public:
      explicit Iterator(const std::vector<size_t>& fst, const std::vector<size_t>& snd, size_t index);

      reference operator*() const;

      Iterator& operator++();
      Iterator operator++(int);

      Iterator& operator--();
      Iterator operator--(int);

      Iterator& operator+=(size_t);
      Iterator& operator-=(size_t);

      Iterator operator+(size_t) const;
      Iterator operator-(size_t) const;

      bool operator==(const Iterator&) const;
      bool operator!=(const Iterator&) const;
    };
  protected:
    std::vector<size_t> _incomingStatic;
    std::vector<size_t> _incomingDynamic;
    std::vector<size_t> _outgoingStatic;
    std::vector<size_t> _outgoingDynamic;
    size_t _id;
    size_t _priority{std::numeric_limits<size_t>::max()};
  public:
    explicit Node(size_t i, size_t prio, std::vector<size_t>&& outStatic);
    explicit Node(size_t i, size_t prio, std::vector<size_t>&& outStatic, std::vector<size_t>&& outDynamic);
    virtual ~Node() = default;
    [[nodiscard]] size_t id() const noexcept;
    [[nodiscard]] size_t priority() const noexcept;
    [[nodiscard]] bool isSource() const noexcept;
    [[nodiscard]] bool isSink() const noexcept;
    [[nodiscard]] bool isDynamic() const noexcept;
    [[nodiscard]] Iterator outgoingBegin() const;
    [[nodiscard]] Iterator outgoingEnd(bool includeDynamic = true) const;
    [[nodiscard]] const std::vector<size_t>& outgoingStatic() const noexcept;
    [[nodiscard]] const std::vector<size_t>& outgoingDynamic() const noexcept;

    void removeOutgoing(unsigned long destination, bool dynamic);
    void removeIncoming(unsigned long origin, bool dynamic);
  };
private:
  std::vector<Node> _nodes;
  std::vector<std::vector<size_t>> _components;

  void scc_util(size_t nodeId, std::vector<Int>& discoverTime,
                std::vector<Int>& lowTime, std::vector<size_t>& stack,
                std::vector<bool>& onStack, Int& time);

  [[nodiscard]] std::vector<size_t> findCycle(const std::vector<size_t>& component,
    size_t componentIndex, const std::vector<size_t>& componentOfVar,
    bool includeDynamic) const;

  std::pair<size_t, size_t> findPivot(std::vector<size_t>& cycle) const;

  const std::vector<std::vector<size_t>>& scc();

  std::vector<std::pair<size_t, size_t>> breakCycles(bool includeDynamic);

public:
  explicit Graph(size_t numNodes);

  void addNode(Node&& node);

  std::vector<std::pair<size_t, size_t>> breakStaticCycles();

  std::vector<std::pair<size_t, size_t>> breakDynamicCycles();

};
}  // namespace atlantis
#pragma once

#include <vector>

namespace utils {

class FlowNetwork {
 private:
  size_t _source;
  size_t _sink;

  std::vector<std::vector<size_t>> _capacities;
  std::vector<std::vector<size_t>> _remainingCapacity;
  std::vector<std::vector<size_t>> _adjacencyList;

 public:
  FlowNetwork(size_t numNodes, size_t source, size_t sink);

  /**
   * Add an edge to this network. If the edge already exists, this operation is
   * ignored. The flow will be initialised to 0.
   *
   * @param from The start of the edge.
   * @param to The end of the edge.
   * @param capacity The capacity of the edge.
   */
  void addEdge(size_t from, size_t to, size_t capacity);

  /**
   * Resets the remaining capacity to full capacity, setting the flow to 0.
   */
  void resetFlows();

  [[nodiscard]] size_t source() const noexcept { return _source; }
  [[nodiscard]] bool isSource(size_t node) const noexcept {
    return _source == node;
  }

  [[nodiscard]] size_t sink() const noexcept { return _sink; }
  [[nodiscard]] size_t size() const noexcept { return _adjacencyList.size(); }

  [[nodiscard]] const std::vector<size_t>& edges(size_t from) const {
    return _adjacencyList[from];
  }

  [[nodiscard]] size_t capacity(size_t from, size_t to) const {
    return _capacities[from][to];
  }

  [[nodiscard]] size_t& remainingCapacity(size_t from, size_t to) {
    return _remainingCapacity[from][to];
  }
};

}  // namespace utils

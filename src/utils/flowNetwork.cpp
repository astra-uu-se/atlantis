#include "utils/flowNetwork.hpp"

#include <cassert>

utils::FlowNetwork::FlowNetwork(size_t numNodes, size_t source, size_t sink) : _source(source), _sink(sink) {
  assert(source < numNodes);
  assert(sink < numNodes);

  _adjacencyList.resize(numNodes);

  _capacities.resize(numNodes);
  _remainingCapacity.resize(numNodes);

  for (auto i = 0u; i < numNodes; i++) {
    _capacities[i].resize(numNodes);
    _remainingCapacity[i].resize(numNodes);
  }
}

void utils::FlowNetwork::addEdge(size_t from, size_t to, size_t capacity) {
  assert(from < size());
  assert(to < size());
  assert(from != _sink);
  assert(to != _source);
  assert(capacity > 0);

  for (auto neighbour : _adjacencyList[from]) {
    if (neighbour == to) {
      return;
    }
  }

  _adjacencyList[from].push_back(to);
  _capacities[from][to] = capacity;
  _capacities[to][from] = 0;
  _remainingCapacity[from][to] = capacity;
  _remainingCapacity[to][from] = 0;
}

void utils::FlowNetwork::resetFlows() {
  for (auto from = 0u; from < size(); from++) {
    for (auto to : _adjacencyList[from]) {
      _remainingCapacity[from][to] = _capacities[from][to];
      _remainingCapacity[to][from] = _capacities[to][from];
    }
  }
}

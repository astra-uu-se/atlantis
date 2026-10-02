#include "atlantis/utils/graph.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/types.hpp"

namespace atlantis {
using Node = Graph::Node;
using Iterator = Graph::Node::Iterator;

Iterator::Iterator(const std::vector<size_t>& fst, const std::vector<size_t>& snd, const size_t index)
    : _first(fst), _second(snd), _index(index) {}

const size_t& Iterator::operator*() const {
  return _index < _first.size() ? _first[_index]
             : _second[_index - _first.size()];
}

Iterator& Iterator::operator++() {
  ++_index;
  return *this;
}

Iterator Iterator::operator++(int) {
  Iterator tmp = *this;
  ++_index;
  return tmp;
}

Iterator& Iterator::operator--() {
  --_index;
  return *this;
}

Iterator Iterator::operator--(int) {
  Iterator tmp = *this;
  --_index;
  return tmp;
}

Iterator& Iterator::operator+=(const size_t offset) {
  _index += static_cast<Int>(offset);
  return *this;
}

Iterator& Iterator::operator-=(const size_t offset) {
  _index -= static_cast<Int>(offset);
  return *this;
}

Iterator Iterator::operator+(const size_t offset) const {
  Iterator tmp = *this;
  tmp._index += static_cast<Int>(offset);
  return tmp;
}

Iterator Iterator::operator-(const size_t offset) const {
  Iterator tmp = *this;
  tmp._index -= static_cast<Int>(offset);
  return tmp;
}

bool Iterator::operator==(const Iterator& other) const {
  return _first == other._first && _second == other._second && _index == other._index;
}

bool Iterator::operator!=(const Iterator& other) const {
  return !operator==(other);
}

inline Node::Node(const size_t i, const size_t prio, std::vector<size_t>&& outStatic) :
_outgoingStatic(std::move(outStatic)),
_id(i),
_priority(prio) {}

Node::Node(const size_t i, const size_t prio, std::vector<size_t>&& outStatic,
           std::vector<size_t>&& outDynamic) :
_outgoingStatic(std::move(outStatic)),
_outgoingDynamic(std::move(outDynamic)),
_id(i),
_priority(prio)
{}

size_t Node::id() const noexcept { return _id; }
size_t Node::priority() const noexcept { return _priority; }

inline bool Node::isSource() const noexcept { return _incomingStatic.empty() && _incomingDynamic.empty(); }

inline bool Node::isSink() const noexcept { return outgoingStatic().empty(); }

bool Node::isDynamic() const noexcept {
  return !_incomingDynamic.empty();
}

Iterator Node::incomingBegin() const {
  return Iterator(_incomingStatic, _incomingDynamic, 0);
}

Iterator Node::incomingEnd(const bool includeDynamic) const {
  return Iterator(_incomingStatic, _incomingDynamic, _incomingStatic.size() + (includeDynamic ? _incomingDynamic.size() : 0));
}

Iterator Node::outgoingBegin() const {
  return Iterator(_outgoingStatic, _outgoingDynamic, 0);
}

Iterator Node::outgoingEnd(const bool includeDynamic) const {
  return Iterator(_outgoingStatic, _outgoingDynamic, _outgoingStatic.size() + (includeDynamic ? _outgoingDynamic.size() : 0));
}

const std::vector<size_t>& Node::outgoingStatic() const noexcept {
  return _outgoingStatic;
}

const std::vector<size_t>& Node::outgoingDynamic() const noexcept {
  return _outgoingDynamic;
}

const std::vector<size_t>& Node::incomingStatic() const noexcept {
  return _incomingStatic;
}

const std::vector<size_t>& Node::incomingDynamic() const noexcept {
  return _incomingDynamic;
}
void Node::removeOutgoing(const size_t destination, bool dynamic) {
  if (dynamic) {
    for (Int i = static_cast<Int>(_outgoingDynamic.size()) - 1; i >= 0; --i) {
      if (_outgoingDynamic[i] == destination) {
        std::swap(_outgoingDynamic[i], _outgoingDynamic.back());
        _outgoingDynamic.pop_back();
      }
    }
    return;
  }
  for (Int i = static_cast<Int>(_outgoingStatic.size()) - 1; i >= 0; --i) {
    if (_outgoingStatic[i] == destination) {
      std::swap(_outgoingStatic[i], _outgoingStatic.back());
      _outgoingStatic.pop_back();
    }
  }
}
void Node::removeIncoming(const size_t origin, const bool dynamic) {
  if (dynamic) {
    for (Int i = static_cast<Int>(_incomingDynamic.size()) - 1; i >= 0; --i) {
      if (_incomingDynamic[i] == origin) {
        std::swap(_incomingDynamic[i], _incomingDynamic.back());
        _incomingDynamic.pop_back();
      }
    }
    return;
  }
  for (Int i = static_cast<Int>(_incomingStatic.size()) - 1; i >= 0; --i) {
    if (_incomingStatic[i] == origin) {
      std::swap(_incomingStatic[i], _incomingStatic.back());
      _incomingStatic.pop_back();
    }
  }
}

void Graph::scc_util(size_t nodeId, std::vector<Int>& discoverTime,
                               std::vector<Int>& lowTime,
                               std::vector<size_t>& stack,
                               std::vector<bool>& onStack, Int& time) {
  assert(nodeId < discoverTime.size());
  assert(discoverTime.size() == lowTime.size());
  assert(discoverTime.size() == onStack.size());
  assert(!onStack[nodeId]);
  discoverTime[nodeId] = lowTime[nodeId] = time;
  ++time;
  stack.emplace_back(nodeId);
  onStack[nodeId] = true;

  for (auto iter = _nodes[nodeId].outgoingBegin(); iter != _nodes[nodeId].outgoingEnd(); ++iter) {
    if (discoverTime[*iter] < 0) {
      scc_util(*iter, discoverTime, lowTime, stack, onStack, time);
      lowTime[nodeId] = std::min(lowTime[*iter], lowTime[nodeId]);
    } else if (onStack[*iter]) {
      lowTime[nodeId] = std::min(discoverTime[*iter], lowTime[nodeId]);
    }
  }
  if (lowTime[nodeId] == discoverTime[nodeId]) {
    const bool in_scc = stack.back() != nodeId;
    if (in_scc) {
      _components.emplace_back();
      while (stack.back() != nodeId) {
        onStack[stack.back()] = false;
        _components.back().emplace_back(stack.back());
        stack.pop_back();
      }
    }
    onStack[nodeId] = false;
    assert(stack.back() == nodeId);
    if (in_scc) {
      _components.back().emplace_back(nodeId);
    }
    stack.pop_back();
  }
}

Graph::Graph(const size_t numNodes) { _nodes.reserve(numNodes); }

void Graph::addNode(Node&& node) {
  assert(node.id() == _nodes.size());
  _nodes.emplace_back(std::move(node));
}

std::vector<std::pair<size_t, size_t>> Graph::breakCycles(const bool includeDynamic) {
  const std::vector<std::vector<size_t>> components = scc();
  if (components.empty()) {
    return {};
  }

  std::vector<size_t> componentOfVar(_nodes.size(), components.size());
  for (size_t c = 0; c < components.size(); ++c) {
    for (const size_t vId : components[c]) {
      componentOfVar[vId] = c;
    }
  }

  std::vector<std::pair<size_t, size_t>> removedArcs;

  for (size_t c = 0; c < components.size(); ++c) {
    while (true) {
      std::vector<size_t> cycle = findCycle(components[c], c, componentOfVar, includeDynamic);
      if (cycle.empty()) {
        break;
      }
      const auto [origin, destination] = findPivot(cycle);
      removedArcs.emplace_back(origin, destination);
      _nodes[origin].removeOutgoing(destination, includeDynamic);
      _nodes[destination].removeIncoming(origin, includeDynamic);
    }
  }
  return removedArcs;
}

std::vector<std::pair<size_t, size_t>> Graph::breakStaticCycles() {
  return breakCycles(false);
}
std::vector<std::pair<size_t, size_t>> Graph::breakDynamicCycles() {
  return breakCycles(true);
}

const std::vector<std::vector<size_t>>& Graph::scc() {
  std::vector<Int> discoverTime(_nodes.size(), -1);
  std::vector<Int> lowTime(_nodes.size(), -1);
  std::vector<size_t> stack;
  stack.reserve(_nodes.size());
  std::vector<bool> onStack(_nodes.size(), false);
  _components.clear();
  _components.reserve(_nodes.size());
  Int time = 0;
  for (size_t nodeId = 0; nodeId < _nodes.size(); ++nodeId) {
    if (_nodes[nodeId].isSource() && discoverTime[nodeId] < 0) {
      scc_util(nodeId, discoverTime, lowTime, stack, onStack, time);
    }
  }
  for (size_t nodeId = 0; nodeId < _nodes.size(); ++nodeId) {
    if (discoverTime[nodeId] < 0) {
      scc_util(nodeId, discoverTime, lowTime, stack, onStack, time);
    }
  }
  assert(std::ranges::none_of(onStack, [&](const bool b) { return b; }));
  assert(
      std::ranges::all_of(discoverTime, [&](const Int t) { return t >= 0; }));
  return _components;
}

std::vector<size_t> Graph::findCycle(const std::vector<size_t>& component,
    const size_t componentIndex, const std::vector<size_t>& componentOfVar,
    const bool includeDynamic) const {
  std::vector<size_t> stack;
  std::vector<Int> discoverTime(componentOfVar.size(), -1);
  std::vector<Int> originOf(componentOfVar.size(), -1);
  stack.reserve(component.size());
  Int time = 0;

  for (const size_t start : component) {
    if (discoverTime[start] >= 0) {
      continue;
    }
    discoverTime[start] = time;
    ++time;
    stack.emplace_back(start);

    while (!stack.empty()) {
      const size_t origin = stack.back();
      stack.pop_back();
      if (origin >= componentOfVar.size()) {
        // This var has been added when breaking a cycle and cannot be in
        // another cycle.
        continue;
      }

      discoverTime[origin] = discoverTime[start];
      if (_nodes[origin].isSource()) {
        continue;
      }
      for (auto iter = _nodes[origin].outgoingBegin(); iter != _nodes[origin].outgoingEnd(includeDynamic); ++iter) {
        if (*iter >= componentOfVar.size() || componentOfVar[*iter] != componentIndex) {
          // This var either: (i) was added when breaking a cycle or (ii) is
          // not in the current component.
          continue;
        }
        originOf[*iter] = static_cast<Int>(origin);
        if (discoverTime[*iter] == discoverTime[start]) {
          std::vector<size_t> cycle;
          cycle.reserve(component.size());
          cycle.emplace_back(*iter);
          for (Int id = static_cast<Int>(origin);
               id != static_cast<Int>(*iter) && id >= 0; id = originOf[id]) {
            assert(id < static_cast<Int>(componentOfVar.size()));
            assert(discoverTime.at(id) == discoverTime.at(start));
            assert(componentOfVar.at(id) == componentOfVar.at(start));
            cycle.emplace_back(id);
          }
          return cycle;
        }
        if (discoverTime[*iter] < 0) {
          stack.emplace_back(*iter);
        }
      }
    }
  }
  return {};
}

std::pair<size_t, size_t> Graph::findPivot(std::vector<size_t>& cycle) const {
  assert(cycle.size() > 1);
  const auto iter = std::ranges::min_element(cycle, [&](const size_t lhs, const size_t rhs) {
    return _nodes[lhs].priority() < _nodes[rhs].priority();
  });
  if (iter == cycle.begin()) {
    return {cycle.back(), cycle.front()};
  }
  return {*(iter - 1), *iter};
}

}  // namespace atlantis
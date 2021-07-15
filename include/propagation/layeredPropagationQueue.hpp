#pragma once
#include <cassert>
#include <memory>
#include <vector>

#include "../core/types.hpp"
#include "../misc/logging.hpp"
#include "utils/propagationListNode.hpp"

class LayeredPropagationQueue {
  typedef PropagationListNode ListNode;

 private:
  ListNode _dummy;
  std::vector<ListNode> _priorities;
  std::vector<ListNode*> _queue;
  size_t _currentLayer;

 public:
  LayeredPropagationQueue() : _dummy(NULL_ID, 123), _priorities(), _queue() {
    _dummy.next = &_dummy;
  }
  // This implementation does not allow copying as this requires special care
  // for repointing all pointers.
  // The current use-case does not require copying anyways.
  LayeredPropagationQueue(const LayeredPropagationQueue&) = delete;
  LayeredPropagationQueue& operator=(const LayeredPropagationQueue&) = delete;

  void init(int numVariables, int numLayers) {
    _priorities.resize(numVariables + 1);
    _queue.resize(numLayers, &_dummy);
    _currentLayer = _queue.size();
  }

  void initVar(VarIdBase id, size_t priority) {
    _priorities[id].id = id;
    _priorities[id].priority = priority;
    _priorities[id].next = nullptr;
  }

  inline bool empty() { return _currentLayer == _queue.size(); }

  inline void push(VarIdBase id) {
    ListNode& node = _priorities[id];
    if (node.next != nullptr) {
      return;  // already enqueued
    }
    node.next = _queue[node.priority];
    _queue[node.priority] = &node;
    _currentLayer = std::min(_currentLayer, node.priority);
  }

  inline VarIdBase pop() {
    ListNode* top = _queue[_currentLayer];
    _queue[_currentLayer] = top->next;
    top->next = nullptr;
    while (_currentLayer < _queue.size() && _queue[_currentLayer] == &_dummy) {
      ++_currentLayer;
    }
    return top->id;
  }

  inline VarIdBase top() { return _queue[_currentLayer]->id; }
};
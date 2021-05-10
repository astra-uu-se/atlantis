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
  ListNode m_dummy;
  std::vector<ListNode> m_priorities;
  std::vector<ListNode*> m_queue;
  size_t m_currentLayer;

 public:
  LayeredPropagationQueue() : m_dummy(NULL_ID, 123), m_priorities(), m_queue() {
    m_dummy.next = &m_dummy;
  }
  // This implementation does not allow copying as this requires special care
  // for repointing all pointers.
  // The current use-case does not require copying anyways.
  LayeredPropagationQueue(const LayeredPropagationQueue&) = delete;
  LayeredPropagationQueue& operator=(const LayeredPropagationQueue&) = delete;

  void init(int numVariables, int numLayers) {
    m_priorities.resize(numVariables + 1);
    m_queue.resize(numLayers, &m_dummy);
    m_currentLayer = m_queue.size();
  }

  void initVar(VarIdBase id, size_t priority) {
    m_priorities[id].id = id;
    m_priorities[id].priority = priority;
    m_priorities[id].next = nullptr;
  }

  bool empty() { return m_currentLayer == m_queue.size(); }
  void push(VarIdBase id) {
    ListNode& node = m_priorities[id];
    if (node.next != nullptr) {
      return;  // already enqueued
    }
    node.next = m_queue[node.priority];
    m_queue[node.priority] = &node;
    m_currentLayer = std::min(m_currentLayer, node.priority);
  }
  VarIdBase pop() {
    ListNode* top = m_queue[m_currentLayer];
    m_queue[m_currentLayer] = top->next;
    top->next = nullptr;
    while (m_currentLayer < m_queue.size() &&
           m_queue[m_currentLayer] == &m_dummy) {
      ++m_currentLayer;
    }
    return top->id;
  }
  VarIdBase top() { return m_queue[m_currentLayer]->id; }
};
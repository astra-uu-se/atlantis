#pragma once
#include <cassert>
#include <memory>
#include <vector>

#include "../core/types.hpp"
#include "core/idMap.hpp"
#include "../misc/logging.hpp"

class PropagationQueue {
  private:
  struct ListNode {
    size_t priority;
    VarIdBase id;
    ListNode* next;
    ListNode(VarIdBase t_id, size_t t_priority)
        : priority(t_priority), id(t_id), next(nullptr) {}
  };

  IdMap<VarIdBase, std::unique_ptr<ListNode>> m_priorityNodes;
  ListNode* head;
  ListNode* tail;

  public:
  PropagationQueue() : m_priorityNodes(0), head(nullptr), tail(nullptr) {}

  // vars must be initialised in order.
  void initVar(VarIdBase id, size_t priority) {
    assert(!m_priorityNodes.has_idx(id));
    m_priorityNodes.register_idx(id);
    m_priorityNodes[id] = std::make_unique<ListNode>(id, priority);
  }

  bool empty() { return head == nullptr; }
  void push(VarIdBase id) {
    ListNode* toInsert = m_priorityNodes[id].get();
    if (toInsert->next != nullptr || tail == toInsert) {
      return;  // id is already is list
    }
    if (head == nullptr) {
      head = toInsert;
      tail = head;
      return;
    }
    // Insert at start of list
    if (toInsert->priority <=
        head->priority) {  // duplicates should not happen but are ok
      toInsert->next = head;
      head = toInsert;
      return;
    }

    // Insert at end of list
    if (toInsert->priority >=
        tail->priority) {  // duplicates should not happen but are ok
      tail->next = toInsert;
      tail = toInsert;
      return;
    }
    ListNode* current = head;
    while (current->next != nullptr) {
      if (current->next->priority >= toInsert->priority) {
        toInsert->next = current->next;
        current->next = toInsert;
        return;
      }
      current = current->next;
    }
    assert(false);  // Insert failed. Cannot happen.
  }
  VarIdBase pop() {
    if (head == nullptr) {
      return NULL_ID;
    }
    ListNode* ret = head;
    if (head == tail) {
      tail = nullptr;
    }
    head = head->next;
    ret->next = nullptr;
    return ret->id;
  }
  VarIdBase top() {
    if (head == nullptr) {
      return NULL_ID;
    }
    return head->id;
  }
};
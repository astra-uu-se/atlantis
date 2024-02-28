#pragma once
#include <cassert>
#include <memory>

#include "propagation/propagation/propagationListNode.hpp"
#include "propagation/types.hpp"
#include "propagation/utils/idMap.hpp"

namespace atlantis::propagation {

class PropagationQueue {
  typedef PropagationListNode ListNode;

 private:
  IdMap<VarIdBase, std::unique_ptr<ListNode>> _priorityNodes;
  ListNode* head;
  ListNode* tail;

 public:
  PropagationQueue() : _priorityNodes(0), head(nullptr), tail(nullptr) {}

  void init(size_t, size_t) {
    _priorityNodes = IdMap<VarIdBase, std::unique_ptr<ListNode>>(0);
    head = nullptr;
    tail = nullptr;
  }

  // vars must be initialised in order.
  void initVar(VarIdBase id, size_t priority) {
    assert(!_priorityNodes.has_idx(id));
    _priorityNodes.register_idx(id);
    _priorityNodes[id] = std::make_unique<ListNode>(id, priority);
  }

  void updatePriority(VarIdBase id, size_t newPriority) {
    assert(_priorityNodes.has_idx(id));
    _priorityNodes[id]->priority = newPriority;
  }

  [[nodiscard]] bool empty() const { return head == nullptr; }

  void push(VarIdBase id) {
    ListNode* toInsert = _priorityNodes[id].get();
    if (toInsert->next != nullptr || head == toInsert || tail == toInsert) {
      return;  // id is already in list
    }
    if (head == nullptr) {
      head = toInsert;
      tail = head;
      return;
    }
    // Insert at start of list (duplicates should not happen but are ok):
    if (toInsert->priority <= head->priority) {
      toInsert->next = head;
      head = toInsert;
      return;
    }

    // Insert at end of list (duplicates should not happen but are ok):
    if (toInsert->priority >= tail->priority) {
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
      assert(current->priority <= current->next->priority);
      current = current->next;
    }
    // Insert failed (this should and cannot happen):
    assert(false);
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

}  // namespace atlantis::propagation
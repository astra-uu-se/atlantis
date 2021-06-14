#pragma once

#include <cassert>
#include <vector>

#include "variables/savedValue.hpp"

class PriorityList {
 private:
  struct DoublyLinkedNode {
    Saved<DoublyLinkedNode*> prev;
    Saved<DoublyLinkedNode*> next;
    Saved<Int> priority;
    DoublyLinkedNode()
        : prev(NULL_TIMESTAMP, nullptr),
          next(NULL_TIMESTAMP, nullptr),
          priority(NULL_TIMESTAMP, 0) {}
    inline void commitIf(Timestamp t) {
      prev.commitIf(t);
      next.commitIf(t);
      priority.commitIf(t);
    }
  };

  std::vector<DoublyLinkedNode> m_list;
  Saved<DoublyLinkedNode*> head;
  Saved<DoublyLinkedNode*> tail;

  inline void unlink(Timestamp t, size_t idx) {
    DoublyLinkedNode* prev = m_list[idx].prev.get(t);
    DoublyLinkedNode* next = m_list[idx].next.get(t);
    if (prev != nullptr) {
      // idx.prev.next = idx.next
      prev->next.set(t, next);
    }
    if (next != nullptr) {
      // idx.next.prev = idx.prev
      next->prev.set(t, prev);
    }
    m_list[idx].prev.set(t, nullptr);
    m_list[idx].next.set(t, nullptr);
  }

 public:
  PriorityList(size_t size)
      : m_list(size),
        head(NULL_TIMESTAMP, nullptr),
        tail(NULL_TIMESTAMP, nullptr) {
    if (size == 0) {
      head.init(NULL_TIMESTAMP, nullptr);
      tail.init(NULL_TIMESTAMP, nullptr);
      return;
    }

    for (size_t i = 0; i < size; ++i) {
      if (i > 0) {
        m_list[i].prev.init(NULL_TIMESTAMP, &m_list[i - 1]);
      }
      if (i < size - 1) {
        m_list[i].next.init(NULL_TIMESTAMP, &m_list[i + 1]);
      }
    }
    head.init(NULL_TIMESTAMP, &m_list[0]);
    tail.init(NULL_TIMESTAMP, &m_list[size - 1]);
  }

  size_t size() { return m_list.size(); }
  Int getMinPriority(Timestamp t) { return head.get(t)->priority.get(t); }
  Int getMaxPriority(Timestamp t) { return tail.get(t)->priority.get(t); }

  void updatePriority(Timestamp t, size_t idx, Int newValue) {
    if (m_list[idx].priority.get(t) == newValue) {
      return;  // No change.
    }

    Int oldValue = m_list[idx].priority.get(t);

    DoublyLinkedNode* currentHead = head.get(t);
    DoublyLinkedNode* currentTail = tail.get(t);

    if (newValue <= currentHead->priority.get(t)) {
      m_list[idx].priority.set(t, newValue);
      if (currentHead == &m_list[idx]) {
        return;
      }
      // idx was tail and will move, so update the tail.
      if (&m_list[idx] == currentTail && currentTail->prev.get(t) != nullptr) {
        tail.set(t, currentTail->prev.get(t));
      }
      // make idx the new current head.
      currentHead->prev.set(t, &m_list[idx]);
      unlink(t, idx);
      m_list[idx].next.set(t, currentHead);
      head.set(t, &m_list[idx]);
      return;
    }

    if (newValue >= currentTail->priority.get(t)) {
      m_list[idx].priority.set(t, newValue);
      if (currentTail == &m_list[idx]) {
        return;
      }
      // idx was head and will move, so update the head.
      if (&m_list[idx] == currentHead && currentHead->next.get(t) != nullptr) {
        head.set(t, currentHead->next.get(t));
      }
      // make idx the new current tail.
      currentTail->next.set(t, &m_list[idx]);
      unlink(t, idx);
      m_list[idx].prev.set(t, currentTail);
      tail.set(t, &m_list[idx]);
      return;
    }

    m_list[idx].priority.set(t, newValue);
    // Check if position is unchanged.
    if (oldValue < newValue) {
      if (newValue <= m_list[idx].next.get(t)->priority.get(t)) {
        return;
      }
    } else {
      if (newValue >= m_list[idx].prev.get(t)->priority.get(t)) {
        return;
      }
    }

    // idx was head/tail and will move, so update the head/tail.
    if (&m_list[idx] == currentHead && currentHead->next.get(t) != nullptr) {
      head.set(t, currentHead->next.get(t));
    }
    if (&m_list[idx] == currentTail && currentTail->prev.get(t) != nullptr) {
      tail.set(t, currentTail->prev.get(t));
    }

    if (oldValue < newValue) {
      // Value increased: insert to the right
      DoublyLinkedNode* current = m_list[idx].next.get(t);
      unlink(t, idx);
      while (newValue > current->priority.get(t)) {
        current = current->next.get(t);
      }
      // Insert before current
      m_list[idx].next.set(t, current);
      m_list[idx].prev.set(t, current->prev.get(t));
      current->prev.get(t)->next.set(t, &m_list[idx]);
      current->prev.set(t, &m_list[idx]);
    } else {
      // Value decreased: insert to the left
      DoublyLinkedNode* current = m_list[idx].prev.get(t);
      unlink(t, idx);
      while (newValue < current->priority.get(t)) {
        current = current->prev.get(t);
      }
      // Insert after current
      m_list[idx].prev.set(t, current);
      m_list[idx].next.set(t, current->next.get(t));
      current->next.get(t)->prev.set(t, &m_list[idx]);
      current->next.set(t, &m_list[idx]);
    }
  }

  void commitIf(Timestamp t) {
    for (auto& node : m_list) {
      node.commitIf(t);
    }
  }

#ifndef NDEBUG
  void sanity(Timestamp t) {
    if (size() == 0) {
      return;
    }
    Int prevPrio = 0;

    size_t s = 0;
    bool first = true;
    Int minPrio = head.get(t)->priority.get(t);
    for (Saved<DoublyLinkedNode*> cur = head; cur.get(t) != nullptr;
         cur = cur.get(t)->next) {
      ++s;
      if (first) {
        prevPrio = cur.get(t)->priority.get(t);
        assert(minPrio == prevPrio);
        first = false;
      } else {
        assert(prevPrio <= cur.get(t)->priority.get(t));
        prevPrio = cur.get(t)->priority.get(t);
      }
    }
    assert(size() == s);

    s = 0;
    first = true;
    Int maxPrio = tail.get(t)->priority.get(t);
    for (Saved<DoublyLinkedNode*> cur = tail; cur.get(t) != nullptr;
         cur = cur.get(t)->prev) {
      ++s;
      if (first) {
        prevPrio = cur.get(t)->priority.get(t);
        assert(maxPrio == prevPrio);
        first = false;
      } else {
        assert(prevPrio >= cur.get(t)->priority.get(t));
        prevPrio = cur.get(t)->priority.get(t);
      }
    }
    assert(size() == s);

    for (Saved<DoublyLinkedNode*> cur = head; cur.get(t) != nullptr;
         cur = cur.get(t)->next) {
      if (cur.get(t)->next.get(t) == nullptr) {
        continue;
      }
      assert(cur.get(t) == cur.get(t)->next.get(t)->prev.get(t));
    }
  }
#endif
};
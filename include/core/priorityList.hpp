#pragma once

#include <memory>
#include <vector>

#include "../core/intVar.hpp"
#include "../core/savedValue.hpp"
#include "../core/types.hpp"

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

   public:
    PriorityList(size_t size)
        : m_list(size),
          head(NULL_TIMESTAMP, nullptr),
          tail(NULL_TIMESTAMP, nullptr) {
      for (size_t i = 0; i < size; ++i) {
        if (i > 0) {
          m_list[i].prev.commitValue(&m_list[i - 1]);
        }
        if (i < size - 1) {
          m_list[i].next.commitValue(&m_list[i + 1]);
        }
      }
      head.commitValue(&m_list[0]);
      tail.commitValue(&m_list[size - 1]);
    }

    Int getMinPriority(Timestamp t) { return head.get(t)->priority.get(t); }
    Int getMaxPriority(Timestamp t) { return tail.get(t)->priority.get(t); }

    void updatePriority(Timestamp t, size_t idx, Int newValue) {
      if (m_list[idx].priority.get(t) == newValue) {
        return;  // No change.
      }

      DoublyLinkedNode* currentHead = head.get(t);
      if (newValue <= currentHead->priority.get(t)) {
        m_list[idx].priority.set(t, newValue);
        if (currentHead == &m_list[idx]) {
          return;
        }
        // make idx the new current head.
        currentHead->prev.set(t, &m_list[idx]);
        unlink(t, idx);
        m_list[idx].next.set(t, currentHead);
        head.set(t, &m_list[idx]);
        return;
      }

      DoublyLinkedNode* currentTail = tail.get(t);
      if (newValue >= currentTail->priority.get(t)) {
        m_list[idx].priority.set(t, newValue);
        if (currentTail == &m_list[idx]) {
          return;
        }
        // make idx the new current tail.
        currentTail->next.set(t, &m_list[idx]);
        unlink(t, idx);
        m_list[idx].prev.set(t, currentTail);
        tail.set(t, &m_list[idx]);
        return;
      }

      Int oldValue = m_list[idx].priority.get(t);
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
      if (&m_list[idx] == head.get(t)) {
        head.set(t, head.get(t)->next.get(t));
      }
      if (&m_list[idx] == tail.get(t)) {
        tail.set(t, tail.get(t)->prev.get(t));
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

    void commitIf(Timestamp t) {
      for (auto& node : m_list) {
        node.commitIf(t);
      }
    }
  };
#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/savedValue.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

/**
 * Invariant for b <- min(X)
 * This version of min should be used when the domain of the variables
 * in X is large compared to the number of variables in X, and/or
 * when few elements in X are expected to take the same value.
 *
 */

class MinSparse : public Invariant {
 private:
  std::vector<VarId> m_X;
  VarId m_b;

  class MinPriorityList {
   private:
    struct DoublyLinkedNode {
      Saved<DoublyLinkedNode*> prev;
      Saved<DoublyLinkedNode*> next;
      Saved<Int> priority;
      DoublyLinkedNode()
          : prev(NULL_TIMESTAMP, nullptr),
            next(NULL_TIMESTAMP, nullptr),
            priority(NULL_TIMESTAMP, 0) {}
      void commitIf(Timestamp t) {
        prev.commitIf(t);
        next.commitIf(t);
        priority.commitIf(t);
      }
    };

    std::vector<DoublyLinkedNode> m_list;
    Saved<DoublyLinkedNode*> head;
    Saved<DoublyLinkedNode*> tail;

   public:
    MinPriorityList(size_t size)
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
        currentTail->next.set(t, &m_list[idx]);
        unlink(t, idx);
        m_list[idx].prev.set(t, currentTail);
        tail.set(t, &m_list[idx]);
        return;
      }
      Int oldPrio = m_list[idx].priority.get(t);
      m_list[idx].priority.set(t, newValue);

      if (oldPrio < newValue) {
        if (newValue <= m_list[idx].next.get(t)->priority.get(t)) {
          return;
        }
      } else {
        if (newValue >= m_list[idx].prev.get(t)->priority.get(t)) {
          return;
        }
      }

      if(&m_list[idx] == head.get(t)) {
        head.set(t, head.get(t)->next.get(t));
      }
      if(&m_list[idx] == tail.get(t)) {
        tail.set(t, tail.get(t)->prev.get(t));
      }

      if (oldPrio < newValue) {
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

  MinPriorityList m_localPriority;

 public:
  MinSparse(std::vector<VarId> X, VarId b);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
};

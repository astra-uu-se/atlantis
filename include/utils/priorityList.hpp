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
    inline void commitIf(Timestamp ts) {
      prev.commitIf(ts);
      next.commitIf(ts);
      priority.commitIf(ts);
    }
  };

  std::vector<DoublyLinkedNode> _list;
  Saved<DoublyLinkedNode*> _head;
  Saved<DoublyLinkedNode*> _tail;

  inline void unlink(Timestamp ts, size_t idx) {
    DoublyLinkedNode* prev = _list[idx].prev.get(ts);
    DoublyLinkedNode* next = _list[idx].next.get(ts);
    if (prev != nullptr) {
      // idx.prev.next = idx.next
      prev->next.set(ts, next);
    }
    if (next != nullptr) {
      // idx.next.prev = idx.prev
      next->prev.set(ts, prev);
    }
    _list[idx].prev.set(ts, nullptr);
    _list[idx].next.set(ts, nullptr);
  }

 public:
  explicit PriorityList(size_t size)
      : _list(size),
        _head(NULL_TIMESTAMP, nullptr),
        _tail(NULL_TIMESTAMP, nullptr) {
    if (size == 0) {
      _head.init(NULL_TIMESTAMP, nullptr);
      _tail.init(NULL_TIMESTAMP, nullptr);
      return;
    }

    for (size_t i = 0; i < size; ++i) {
      if (i > 0) {
        _list[i].prev.init(NULL_TIMESTAMP, &_list[i - 1]);
      }
      if (i < size - 1) {
        _list[i].next.init(NULL_TIMESTAMP, &_list[i + 1]);
      }
    }
    _head.init(NULL_TIMESTAMP, &_list[0]);
    _tail.init(NULL_TIMESTAMP, &_list[size - 1]);
  }

  size_t size() { return _list.size(); }
  Int getMinPriority(Timestamp ts) { return _head.get(ts)->priority.get(ts); }
  Int getMaxPriority(Timestamp ts) { return _tail.get(ts)->priority.get(ts); }

  void updatePriority(Timestamp ts, size_t idx, Int newValue) {
    if (_list[idx].priority.get(ts) == newValue) {
      return;  // No change.
    }

    const Int oldValue = _list[idx].priority.get(ts);

    DoublyLinkedNode* currentHead = _head.get(ts);
    DoublyLinkedNode* currentTail = _tail.get(ts);

    if (newValue <= currentHead->priority.get(ts)) {
      _list[idx].priority.set(ts, newValue);
      if (currentHead == &_list[idx]) {
        return;
      }
      // idx was _tail and will move, so update the _tail.
      if (&_list[idx] == currentTail && currentTail->prev.get(ts) != nullptr) {
        _tail.set(ts, currentTail->prev.get(ts));
      }
      // make idx the new current _head.
      currentHead->prev.set(ts, &_list[idx]);
      unlink(ts, idx);
      _list[idx].next.set(ts, currentHead);
      _head.set(ts, &_list[idx]);
      return;
    }

    if (newValue >= currentTail->priority.get(ts)) {
      _list[idx].priority.set(ts, newValue);
      if (currentTail == &_list[idx]) {
        return;
      }
      // idx was _head and will move, so update the _head.
      if (&_list[idx] == currentHead && currentHead->next.get(ts) != nullptr) {
        _head.set(ts, currentHead->next.get(ts));
      }
      // make idx the new current _tail.
      currentTail->next.set(ts, &_list[idx]);
      unlink(ts, idx);
      _list[idx].prev.set(ts, currentTail);
      _tail.set(ts, &_list[idx]);
      return;
    }

    _list[idx].priority.set(ts, newValue);
    // Check if position is unchanged.
    if (oldValue < newValue) {
      if (newValue <= _list[idx].next.get(ts)->priority.get(ts)) {
        return;
      }
    } else {
      if (newValue >= _list[idx].prev.get(ts)->priority.get(ts)) {
        return;
      }
    }

    // idx was _head/_tail and will move, so update the _head/_tail.
    if (&_list[idx] == currentHead && currentHead->next.get(ts) != nullptr) {
      _head.set(ts, currentHead->next.get(ts));
    }
    if (&_list[idx] == currentTail && currentTail->prev.get(ts) != nullptr) {
      _tail.set(ts, currentTail->prev.get(ts));
    }

    if (oldValue < newValue) {
      // Value increased: insert to the right
      DoublyLinkedNode* current = _list[idx].next.get(ts);
      unlink(ts, idx);
      while (newValue > current->priority.get(ts)) {
        current = current->next.get(ts);
      }
      // Insert before current
      _list[idx].next.set(ts, current);
      _list[idx].prev.set(ts, current->prev.get(ts));
      current->prev.get(ts)->next.set(ts, &_list[idx]);
      current->prev.set(ts, &_list[idx]);
    } else {
      // Value decreased: insert to the left
      DoublyLinkedNode* current = _list[idx].prev.get(ts);
      unlink(ts, idx);
      while (newValue < current->priority.get(ts)) {
        current = current->prev.get(ts);
      }
      // Insert after current
      _list[idx].prev.set(ts, current);
      _list[idx].next.set(ts, current->next.get(ts));
      current->next.get(ts)->prev.set(ts, &_list[idx]);
      current->next.set(ts, &_list[idx]);
    }
  }

  void commitIf(Timestamp ts) {
    for (auto& node : _list) {
      node.commitIf(ts);
    }
  }

#ifdef CBLS_TEST
  void sanity(Timestamp ts) {
    if (size() == 0) {
      return;
    }
    Int prevPrio = 0;

    size_t s = 0;
    bool first = true;
    const Int minPrio = _head.get(ts)->priority.get(ts);
    for (Saved<DoublyLinkedNode*> cur = _head; cur.get(ts) != nullptr;
         cur = cur.get(ts)->next) {
      ++s;
      if (first) {
        prevPrio = cur.get(ts)->priority.get(ts);
        assert(minPrio == prevPrio);
        first = false;
      } else {
        assert(prevPrio <= cur.get(ts)->priority.get(ts));
        prevPrio = cur.get(ts)->priority.get(ts);
      }
    }
    assert(size() == s);

    s = 0;
    first = true;
    Int maxPrio = _tail.get(ts)->priority.get(ts);
    for (Saved<DoublyLinkedNode*> cur = _tail; cur.get(ts) != nullptr;
         cur = cur.get(ts)->prev) {
      ++s;
      if (first) {
        prevPrio = cur.get(ts)->priority.get(ts);
        assert(maxPrio == prevPrio);
        first = false;
      } else {
        assert(prevPrio >= cur.get(ts)->priority.get(ts));
        prevPrio = cur.get(ts)->priority.get(ts);
      }
    }
    assert(size() == s);

    for (Saved<DoublyLinkedNode*> cur = _head; cur.get(ts) != nullptr;
         cur = cur.get(ts)->next) {
      if (cur.get(ts)->next.get(ts) == nullptr) {
        continue;
      }
      assert(cur.get(ts) == cur.get(ts)->next.get(ts)->prev.get(ts));
    }
  }
#endif
};
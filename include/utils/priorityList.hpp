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

  std::vector<DoublyLinkedNode> _list;
  Saved<DoublyLinkedNode*> _head;
  Saved<DoublyLinkedNode*> _tail;

  inline void unlink(Timestamp t, size_t idx) {
    DoublyLinkedNode* prev = _list[idx].prev.get(t);
    DoublyLinkedNode* next = _list[idx].next.get(t);
    if (prev != nullptr) {
      // idx.prev.next = idx.next
      prev->next.set(t, next);
    }
    if (next != nullptr) {
      // idx.next.prev = idx.prev
      next->prev.set(t, prev);
    }
    _list[idx].prev.set(t, nullptr);
    _list[idx].next.set(t, nullptr);
  }

 public:
  PriorityList(size_t size)
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
  Int getMinPriority(Timestamp t) { return _head.get(t)->priority.get(t); }
  Int getMaxPriority(Timestamp t) { return _tail.get(t)->priority.get(t); }

  void updatePriority(Timestamp t, size_t idx, Int newValue) {
    if (_list[idx].priority.get(t) == newValue) {
      return;  // No change.
    }

    Int oldValue = _list[idx].priority.get(t);

    DoublyLinkedNode* currentHead = _head.get(t);
    DoublyLinkedNode* currentTail = _tail.get(t);

    if (newValue <= currentHead->priority.get(t)) {
      _list[idx].priority.set(t, newValue);
      if (currentHead == &_list[idx]) {
        return;
      }
      // idx was _tail and will move, so update the _tail.
      if (&_list[idx] == currentTail && currentTail->prev.get(t) != nullptr) {
        _tail.set(t, currentTail->prev.get(t));
      }
      // make idx the new current _head.
      currentHead->prev.set(t, &_list[idx]);
      unlink(t, idx);
      _list[idx].next.set(t, currentHead);
      _head.set(t, &_list[idx]);
      return;
    }

    if (newValue >= currentTail->priority.get(t)) {
      _list[idx].priority.set(t, newValue);
      if (currentTail == &_list[idx]) {
        return;
      }
      // idx was _head and will move, so update the _head.
      if (&_list[idx] == currentHead && currentHead->next.get(t) != nullptr) {
        _head.set(t, currentHead->next.get(t));
      }
      // make idx the new current _tail.
      currentTail->next.set(t, &_list[idx]);
      unlink(t, idx);
      _list[idx].prev.set(t, currentTail);
      _tail.set(t, &_list[idx]);
      return;
    }

    _list[idx].priority.set(t, newValue);
    // Check if position is unchanged.
    if (oldValue < newValue) {
      if (newValue <= _list[idx].next.get(t)->priority.get(t)) {
        return;
      }
    } else {
      if (newValue >= _list[idx].prev.get(t)->priority.get(t)) {
        return;
      }
    }

    // idx was _head/_tail and will move, so update the _head/_tail.
    if (&_list[idx] == currentHead && currentHead->next.get(t) != nullptr) {
      _head.set(t, currentHead->next.get(t));
    }
    if (&_list[idx] == currentTail && currentTail->prev.get(t) != nullptr) {
      _tail.set(t, currentTail->prev.get(t));
    }

    if (oldValue < newValue) {
      // Value increased: insert to the right
      DoublyLinkedNode* current = _list[idx].next.get(t);
      unlink(t, idx);
      while (newValue > current->priority.get(t)) {
        current = current->next.get(t);
      }
      // Insert before current
      _list[idx].next.set(t, current);
      _list[idx].prev.set(t, current->prev.get(t));
      current->prev.get(t)->next.set(t, &_list[idx]);
      current->prev.set(t, &_list[idx]);
    } else {
      // Value decreased: insert to the left
      DoublyLinkedNode* current = _list[idx].prev.get(t);
      unlink(t, idx);
      while (newValue < current->priority.get(t)) {
        current = current->prev.get(t);
      }
      // Insert after current
      _list[idx].prev.set(t, current);
      _list[idx].next.set(t, current->next.get(t));
      current->next.get(t)->prev.set(t, &_list[idx]);
      current->next.set(t, &_list[idx]);
    }
  }

  void commitIf(Timestamp t) {
    for (auto& node : _list) {
      node.commitIf(t);
    }
  }

  void sanity(Timestamp t) {
    if (size() == 0) {
      return;
    }
    Int prevPrio = 0;

    size_t s = 0;
    bool first = true;
    Int minPrio = _head.get(t)->priority.get(t);
    for (Saved<DoublyLinkedNode*> cur = _head; cur.get(t) != nullptr;
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
    Int maxPrio = _tail.get(t)->priority.get(t);
    for (Saved<DoublyLinkedNode*> cur = _tail; cur.get(t) != nullptr;
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

    for (Saved<DoublyLinkedNode*> cur = _head; cur.get(t) != nullptr;
         cur = cur.get(t)->next) {
      if (cur.get(t)->next.get(t) == nullptr) {
        continue;
      }
      assert(cur.get(t) == cur.get(t)->next.get(t)->prev.get(t));
    }
  }
};
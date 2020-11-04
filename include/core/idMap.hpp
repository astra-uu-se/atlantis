#pragma once
#include <cassert>
#include <iostream>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "types.hpp"

// TODO: do template specialisation for when T is also an ID so that NULL_ID is
// then the default pushed into the stack and delete the default constructors
// for IDs
template <typename I, typename T>
class IdMap {
 private:
  std::vector<T> m_vector;

 public:
  IdMap(size_t reservedSize) {
    static_assert(std::is_base_of<Id, I>::value,
                  "The index must be a subclass of id");
    m_vector.reserve(reservedSize);
  }

  inline T& operator[](I idx) {
    auto i = static_cast<Id>(idx).id;
    assert(i > 0);
    assert(i <= m_vector.size());
    return m_vector[i - 1];
  }

  inline const T& at(I idx) const {
    auto i = static_cast<Id>(idx).id;
    assert(i > 0);
    assert(i <= m_vector.size());
    return m_vector[i - 1];
  }

  // inline void push_back(T data) { m_vector.push_back(data); }

  inline void register_idx(I idx) {
    if (static_cast<size_t>(idx.id) != m_vector.size() + 1) {
      throw OutOfOrderIndexRegistration();
    }
    m_vector.emplace_back(T());
  }

  inline void register_idx(I idx, T initValue) {
    if (static_cast<size_t>(idx.id) != m_vector.size() + 1) {
      throw OutOfOrderIndexRegistration();
    }
    m_vector.emplace_back(initValue);
  }

  inline void assign_all(T value) { m_vector.assign(m_vector.size(), value); }

  inline size_t size() const { return m_vector.size(); }
  typedef typename std::vector<T>::iterator iterator;

  inline iterator begin() { return m_vector.begin(); }
  inline iterator end() { return m_vector.end(); }
};

template <typename I>
class IdMap<I, bool> {
 private:
  std::vector<bool> m_vector;

 public:
  IdMap(size_t reservedSize) {
    static_assert(std::is_base_of<Id, I>::value,
                  "The index must be a subclass of id");
    m_vector.reserve(reservedSize);
  }

  inline bool get(I idx) const {
    auto i = static_cast<Id>(idx).id;
    assert(i > 0);
    assert(i <= m_vector.size());
    return m_vector[i - 1];
  }

  inline void set(I idx, bool value) {
    auto i = static_cast<Id>(idx).id;
    assert(i > 0);
    assert(i <= m_vector.size());
    m_vector[i - 1] = value;
  }

  inline void register_idx(I idx, bool initValue) {
    if (static_cast<size_t>(idx.id) != m_vector.size() + 1) {
      throw OutOfOrderIndexRegistration();
    }
    m_vector.emplace_back(initValue);
  }

  inline void assign_all(bool value) {
    m_vector.assign(m_vector.size(), value);
  }

  void print() {
    for (auto foo : m_vector) {
      std::cout << foo << "\n";
    }
  }
};
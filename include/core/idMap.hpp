#pragma once
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

  inline T& at(I idx) { return m_vector.at(static_cast<Id>(idx).id - 1); }

  // inline void push_back(T data) { m_vector.push_back(data); }

  inline void register_idx(I idx) {
    if (static_cast<size_t>(idx.id) != m_vector.size() + 1) {
      throw OutOfOrderIndexRegistration();
    }
    m_vector.push_back(T());
  }

  void print() {
    for (auto foo : m_vector) {
      std::cout << foo << "\n";
    }
  }
};
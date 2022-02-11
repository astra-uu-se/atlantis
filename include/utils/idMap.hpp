#pragma once
#include <cassert>
#include <vector>

#include "core/types.hpp"
#include "exceptions/exceptions.hpp"

// TODO: do template specialisation for when T is also an ID so that NULL_ID is
// then the default pushed into the stack and delete the default constructors
// for IDs
template <typename I, typename T>
class IdMap {
 private:
  std::vector<T> _vector;

 public:
  explicit IdMap(size_t reservedSize) {
    static_assert(std::is_base_of<Id, I>::value,
                  "The index must be a subclass of id");
    _vector.reserve(reservedSize);
  }

  inline T& operator[](I idx) {
    assert(static_cast<Id>(idx).id > 0);
    assert(static_cast<Id>(idx).id <= _vector.size());
    return _vector[static_cast<Id>(idx).id - 1];
  }

  inline const T& at(I idx) const {
    assert(static_cast<Id>(idx).id > 0);
    assert(static_cast<Id>(idx).id <= _vector.size());
    return _vector[static_cast<Id>(idx).id - 1];
  }

  inline void register_idx(I idx) {
    if (static_cast<size_t>(idx.id) != _vector.size() + 1) {
      throw OutOfOrderIndexRegistration();
    }
    _vector.emplace_back(T());
  }

  inline void register_idx(I idx, T initValue) {
    if (static_cast<size_t>(idx.id) != _vector.size() + 1) {
      throw OutOfOrderIndexRegistration();
    }
    _vector.emplace_back(initValue);
  }

  inline void register_idx_move(I idx, T&& initValue) {
    if (static_cast<size_t>(idx.id) != _vector.size() + 1) {
      throw OutOfOrderIndexRegistration();
    }
    _vector.emplace_back(std::move(initValue));
  }

  inline void assign_all(T value) { _vector.assign(_vector.size(), value); }

  [[nodiscard]] inline size_t size() const { return _vector.size(); }
  [[nodiscard]] inline bool has_idx(I idx) {
    return static_cast<size_t>(idx.id) <= _vector.size();
  }
  typedef typename std::vector<T>::iterator iterator;

  inline iterator begin() { return _vector.begin(); }
  inline iterator end() { return _vector.end(); }
};

template <typename I>
class IdMap<I, bool> {
 private:
  std::vector<bool> _vector;

 public:
  explicit IdMap(size_t reservedSize) {
    static_assert(std::is_base_of<Id, I>::value,
                  "The index must be a subclass of id");
    _vector.reserve(reservedSize);
  }

  inline bool get(I idx) const {
    assert(static_cast<Id>(idx).id > 0);
    assert(static_cast<Id>(idx).id <= _vector.size());
    return _vector[static_cast<Id>(idx).id - 1];
  }

  inline void set(I idx, bool value) {
    assert(static_cast<Id>(idx).id > 0);
    assert(static_cast<Id>(idx).id <= _vector.size());
    _vector[static_cast<Id>(idx).id - 1] = value;
  }

  inline void register_idx(I idx, bool initValue) {
    if (static_cast<size_t>(idx.id) != _vector.size() + 1) {
      throw OutOfOrderIndexRegistration();
    }
    _vector.emplace_back(initValue);
  }

  inline void assign_all(bool value) { _vector.assign(_vector.size(), value); }

  // std::string toString() {
  //   std::string str = "";
  //   for (const auto foo : _vector) {
  //     str += foo + "\n";
  //   }
  //   return str;
  // }
};
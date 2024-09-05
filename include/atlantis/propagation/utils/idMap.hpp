#pragma once
#include <cassert>
#include <vector>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

// TODO: do template specialisation for when T is also an ID so that NULL_ID is
// then the default pushed into the stack and delete the default constructors
// for IDs
template <typename T>
class IdMap {
 private:
  std::vector<T> _vector;

 public:
  explicit IdMap(size_t reservedSize) { _vector.reserve(reservedSize); }

  inline T& operator[](size_t idx) {
    assert(idx < _vector.size());
    return _vector[idx];
  }

  inline const T& at(size_t idx) const {
    assert(idx < _vector.size());
    return _vector[idx];
  }

  inline void register_idx(size_t idx) {
    if (idx != _vector.size()) {
      throw OutOfOrderIndexRegistration();
    }
    _vector.emplace_back(T());
  }

  inline void register_idx(size_t idx, T initValue) {
    if (idx != _vector.size()) {
      throw OutOfOrderIndexRegistration();
    }
    _vector.emplace_back(initValue);
  }

  inline void register_idx_move(size_t idx, T&& initValue) {
    if (idx != _vector.size()) {
      throw OutOfOrderIndexRegistration();
    }
    _vector.emplace_back(std::move(initValue));
  }

  inline void assign_all(T value) { _vector.assign(_vector.size(), value); }

  [[nodiscard]] inline size_t size() const { return _vector.size(); }
  [[nodiscard]] inline bool has_idx(size_t idx) { return idx < _vector.size(); }
  typedef typename std::vector<T>::iterator iterator;

  inline void clear() { _vector.clear(); }

  inline iterator begin() { return _vector.begin(); }
  inline iterator end() { return _vector.end(); }
};

template <>
class IdMap<bool> {
 private:
  std::vector<bool> _vector;

 public:
  explicit IdMap(size_t reservedSize) { _vector.reserve(reservedSize); }

  inline bool get(size_t idx) const {
    assert(idx < _vector.size());
    return _vector[idx];
  }

  inline void set(size_t idx, bool value) {
    assert(idx < _vector.size());
    _vector[idx] = value;
  }

  inline void register_idx(size_t idx, bool initValue) {
    if (idx != _vector.size()) {
      throw OutOfOrderIndexRegistration();
    }
    _vector.emplace_back(initValue);
  }

  inline void clear() { _vector.clear(); }

  inline void assign_all(bool value) { _vector.assign(_vector.size(), value); }

  inline void assign(size_t newSize, bool value) {
    _vector.assign(newSize, value);
  }

  // std::string toString() {
  //   std::string str = "";
  //   for (const auto foo : _vector) {
  //     str += foo + "\n";
  //   }
  //   return str;
  // }
};

}  // namespace atlantis::propagation

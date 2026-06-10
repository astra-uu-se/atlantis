#include "atlantis/sortedUniqueVector.hpp"

#include <algorithm>

namespace atlantis {

std::vector<Int> sortedUniqueVector(std::vector<Int>&& vec) {
  std::ranges::sort(vec);
  const auto [first, last] = std::ranges::unique(vec);
  vec.erase(first, last);
  return vec;
}

SortedUniqueVector::SortedUniqueVector(std::vector<Int>&& vec)
    : _vec(sortedUniqueVector(std::move(vec))) {}

bool SortedUniqueVector::isInterval() const noexcept {
  if (_vec.empty()) {
    return true;
  }
  return _vec.back() - _vec.front() == static_cast<Int>(_vec.size()) - 1;
}
const std::vector<Int>& SortedUniqueVector::operator*() const noexcept {
  return _vec;
}
std::vector<Int> const* SortedUniqueVector::operator->() const noexcept {
  return &_vec;
}
Int SortedUniqueVector::operator[](const size_t index) const noexcept {
  return _vec[index];
}

}  // namespace atlantis

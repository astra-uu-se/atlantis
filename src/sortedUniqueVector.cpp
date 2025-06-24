#include "atlantis/sortedUniqueVector.hpp"
#include <algorithm>

namespace atlantis {

std::vector<Int> sortedUniqueVector(std::vector<Int>&& vec) {
  std::ranges::sort(vec);
  const auto [first, last] = std::ranges::unique(vec);
  vec.erase(first, last);
  return vec;
}

SortedUniqueVector::SortedUniqueVector(std::vector<Int>&& v) :
  vec(sortedUniqueVector(std::move(v))) {}

bool SortedUniqueVector::isInterval() const noexcept {
  if (vec.empty()) {
    return true;
  }
  return vec.back() - vec.front() == static_cast<int>(vec.size()) - 1;
}


const std::vector<Int>& SortedUniqueVector::operator*() const {
  return vec;
}

}  // namespace atlantis

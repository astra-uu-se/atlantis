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

const std::vector<Int>& SortedUniqueVector::operator*() const {
  return vec;
}

}  // namespace atlantis

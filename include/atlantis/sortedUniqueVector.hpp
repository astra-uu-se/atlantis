#pragma once

#include <vector>

#include "atlantis/types.hpp"

namespace atlantis {

class SortedUniqueVector {
  const std::vector<Int> vec;

 public:
  SortedUniqueVector(SortedUniqueVector&) = default;
  SortedUniqueVector(const SortedUniqueVector&) = default;
  SortedUniqueVector(SortedUniqueVector&&) = default;
  explicit SortedUniqueVector(std::vector<Int>&& vec);
  [[nodiscard]] bool isInterval() const noexcept;
  const std::vector<Int>& operator*() const noexcept;
};

}  // namespace atlantis
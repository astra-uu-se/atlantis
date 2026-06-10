#pragma once

#include <vector>

#include "atlantis/types.hpp"

namespace atlantis {

class SortedUniqueVector {
  const std::vector<Int> _vec;

 public:
  SortedUniqueVector(SortedUniqueVector&) = default;
  SortedUniqueVector(const SortedUniqueVector&) = default;
  SortedUniqueVector(SortedUniqueVector&&) = default;
  explicit SortedUniqueVector(std::vector<Int>&& vec);
  [[nodiscard]] bool isInterval() const noexcept;

  const std::vector<Int>& operator*() const noexcept;
  std::vector<Int> const* operator->() const noexcept;
  [[nodiscard]] Int operator[](size_t) const noexcept;
};

}  // namespace atlantis
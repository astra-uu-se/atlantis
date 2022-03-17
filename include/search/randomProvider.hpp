#pragma once

#include <optional>
#include <random>
#include <vector>

#include "core/types.hpp"

namespace search {

class RandomProvider {
 public:
  explicit RandomProvider(int seed) : _gen(std::mt19937(seed)) {}

  template <typename T>
  std::optional<T> element(const std::vector<T> collection) {
    if (collection.empty()) {
      return std::nullopt;
    }

    std::uniform_int_distribution<size_t> distribution(0,
                                                       collection.size() - 1);
    return collection[distribution(_gen)];
  }

  Int intInRange(Int lowerBound, Int upperBound) {
    std::uniform_int_distribution<Int> distribution(lowerBound, upperBound);
    return distribution(_gen);
  }

 private:
  std::mt19937 _gen;
};

}  // namespace search

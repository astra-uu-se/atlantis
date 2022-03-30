#pragma once

#include <optional>
#include <random>
#include <vector>

#include "core/types.hpp"

namespace search {

class RandomProvider {
 private:
  std::mt19937 _gen;

 public:
  explicit RandomProvider(std::uint_fast32_t seed) : _gen(std::mt19937(seed)) {}

  template <typename T>
  const T& element(const std::vector<T>& collection) {
    assert(!collection.empty());
    std::uniform_int_distribution<size_t> distribution(0,
                                                       collection.size() - 1);
    return collection[distribution(_gen)];
  }

  Int intInRange(Int lowerBound, Int upperBound) {
    return std::uniform_int_distribution<Int>(lowerBound, upperBound)(_gen);
  }

  float floatInRange(float lowerBound, float upperBound) {
    return std::uniform_real_distribution<float>(lowerBound, upperBound)(_gen);
  }
};

}  // namespace search

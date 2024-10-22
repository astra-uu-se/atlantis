#pragma once

#include <optional>
#include <random>
#include <vector>

#include "atlantis/types.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search {

class RandomProvider {
 private:
  std::mt19937 _gen;
  std::default_random_engine _randomEngine;

 public:
  explicit RandomProvider(std::uint_fast32_t seed) : _gen(std::mt19937(seed)) {}

  template <typename T>
  T& element(std::vector<T>& collection) {
    assert(!collection.empty());
    std::uniform_int_distribution<size_t> distribution(0,
                                                       collection.size() - 1);
    return collection[distribution(_gen)];
  }

  template <typename T>
  const T& element(const std::vector<T>& collection) {
    assert(!collection.empty());
    std::uniform_int_distribution<size_t> distribution(0,
                                                       collection.size() - 1);
    return collection[distribution(_gen)];
  }

  template <typename Iter>
  Iter iterator(Iter begin, Iter end) {
    auto offset = intInRange(0, std::distance(begin, end) - 1);
    return std::next(begin, offset);
  }

  Int intInRange(Int lowerBound, Int upperBound) {
    return std::uniform_int_distribution<Int>(lowerBound, upperBound)(_gen);
  }

  float floatInRange(float lowerBound, float upperBound) {
    return std::uniform_real_distribution<float>(lowerBound, upperBound)(_gen);
  }

  Int inDomain(const SetDomain& domain) { return element(domain.values()); }

  Int inDomain(const IntervalDomain& domain) {
    return intInRange(domain.lowerBound(), domain.upperBound());
  }

  Int inDomain(SearchDomain& domain) {
    return std::visit<Int>([&](const auto& dom) { return inDomain(dom); },
                           domain.innerDomain());
  }

  template <typename T>
  void shuffle(std::vector<T>& v) {
    std::shuffle(v.begin(), v.end(), _randomEngine);
  }

  void seed(std::int_fast32_t seed) { _gen.seed(seed); }

  template <typename Value, typename Distribution>
  Value fromDistribution(Distribution d) {
    return d(_gen);
  }
};

}  // namespace atlantis::search

#pragma once

#include <algorithm>
#include <random>
#include <vector>

#include "atlantis/types.hpp"

namespace atlantis {
class SetDomain;
class IntervalDomain;
class SearchDomain;
}  // namespace atlantis

namespace atlantis::search {

class RandomProvider {
  std::mt19937 _gen;
  std::default_random_engine _randomEngine;

 public:
  explicit RandomProvider(std::uint_fast32_t seed);

  Int element(const std::vector<Int>& collection);

  bool boolean();

  Int intInRange(Int lowerBound, Int upperBound);

  Int intInRange(Int lowerBound, Int upperBound, Int ignoredValue);

  float floatInRange(float lowerBound, float upperBound);

  Int inDomain(const SetDomain& domain);

  Int inDomain(const SetDomain& domain, Int ignoredValue);

  Int inDomain(const IntervalDomain& domain);

  Int inDomain(const IntervalDomain& domain, Int ignoredValue);

  Int inDomain(const SearchDomain& domain);

  Int inDomain(const SearchDomain& domain, Int ignoredValue);

  void seed(std::int_fast32_t seed);

  template <typename T>
  void shuffle(std::vector<T>& v) {
    std::ranges::shuffle(v.begin(), v.end(), _randomEngine);
  }

  template <typename Value, typename Distribution>
  Value fromDistribution(Distribution d) {
    return d(_gen);
  }
};

}  // namespace atlantis::search

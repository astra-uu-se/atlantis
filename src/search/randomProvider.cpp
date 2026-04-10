#include "atlantis/search/randomProvider.hpp"

#include <fznparser/model.hpp>

#include "atlantis/utils/domains.hpp"

namespace atlantis::search {

RandomProvider::RandomProvider(const std::uint_fast32_t seed)
    : _gen(std::mt19937(seed)) {}

Int RandomProvider::element(const std::vector<Int>& collection) {
  assert(!collection.empty());
  std::uniform_int_distribution<size_t> distribution(0, collection.size() - 1);
  return collection[distribution(_gen)];
}

bool RandomProvider::boolean() {
  return std::uniform_int_distribution<Int>(0, 1)(_gen) == 1;
}

Int RandomProvider::intInRange(const Int lowerBound, const Int upperBound) {
  return std::uniform_int_distribution<Int>(lowerBound, upperBound)(_gen);
}

Int RandomProvider::intInRange(const Int lowerBound, const Int upperBound,
                               const Int ignoredValue) {
  assert(lowerBound < upperBound);
  assert(lowerBound <= ignoredValue && ignoredValue <= upperBound);
  const Int val =
      std::uniform_int_distribution<Int>(lowerBound, upperBound - 1)(_gen);
  return val == ignoredValue ? upperBound : val;
}

float RandomProvider::floatInRange(const float lowerBound,
                                   const float upperBound) {
  return std::uniform_real_distribution<float>(lowerBound, upperBound)(_gen);
}

Int RandomProvider::inDomain(const SetDomain& domain) {
  return domain[std::uniform_int_distribution<size_t>(0,
                                                      domain.size() - 1)(_gen)];
}

Int RandomProvider::inDomain(const SetDomain& domain, const Int ignoredValue) {
  assert(domain.contains(ignoredValue));
  const size_t index =
      std::uniform_int_distribution<size_t>(0, domain.size() - 2)(_gen);
  const Int val = domain[index];
  return val == ignoredValue ? domain.upperBound() : val;
}

Int RandomProvider::inDomain(const IntervalDomain& domain) {
  return intInRange(domain.lowerBound(), domain.upperBound());
}

Int RandomProvider::inDomain(const IntervalDomain& domain,
                             const Int ignoredValue) {
  return intInRange(domain.lowerBound(), domain.upperBound(), ignoredValue);
}

Int RandomProvider::inDomain(const SearchDomain& domain) {
  return domain[intInRange(0, static_cast<Int>(domain.size()) - 1)];
}

Int RandomProvider::inDomain(const SearchDomain& domain,
                             const Int ignoredValue) {
  assert(domain.contains(ignoredValue));
  const size_t index =
      std::uniform_int_distribution<size_t>(0, domain.size() - 2)(_gen);
  const Int val = domain[index];
  return val == ignoredValue ? domain.upperBound() : val;
}

void RandomProvider::seed(const std::int_fast32_t seed) { _gen.seed(seed); }

}  // namespace atlantis::search
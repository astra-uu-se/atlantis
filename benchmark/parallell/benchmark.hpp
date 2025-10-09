#pragma once

#include <functional>

#include "atlantis/types.hpp"

namespace atlantis::benchmark {

inline search::SearchType intToSearchType(Int state) {
  switch (state) {
    case 2:
      return search::SearchType::BEAMSEARCH;
    case 1:
      return search::SearchType::PARALLEL;
    case 0:
    default:
      return search::SearchType::BESTCOST;
  }
}

inline void defaultArguments(::benchmark::internal::Benchmark* benchmark) {
  for (const Int timelimit : std::array{5000, 30000, 60000, 180000}) {
    for (Int numThreads = 2; numThreads <= 16; numThreads *= 2) {
      for (Int searchType = 0; searchType <= 2; ++searchType) {
        benchmark->Args({timelimit, numThreads, searchType});
      }
#ifndef NDEBUG
      return;
#endif
    }
  }
}

}  // namespace atlantis::benchmark

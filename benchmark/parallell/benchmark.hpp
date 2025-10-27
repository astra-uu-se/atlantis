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

template <class F>
inline void defaultArguments(::benchmark::internal::Benchmark* benchmark) {
  F::populateInstances();
  for (size_t instance = 0; instance < F::size(); ++instance) {
    for (const Int timelimit : std::array{5000, 30000, 60000, 180000}) {
      for (Int numThreads = 1; numThreads <= 32; numThreads *= 2) {
        for (Int searchType = 0; searchType <= 2; ++searchType) {
          benchmark->Args({static_cast<long>(instance), timelimit, numThreads, searchType});
          if (numThreads == 1) {
            break;
          }
        }
#ifndef NDEBUG
        if (numThreads >= 2) {
          return;
        }
#endif
      }
    }
  }
}

inline std::vector<std::string> createInstances(const std::string& relDir) {
  std::vector<std::string> instances;
  if (!std::filesystem::exists(relDir)) {
    return {};
  }
  for (const auto& entry : std::filesystem::directory_iterator(relDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".fzn") {
      const std::string dirPath = entry.path().parent_path().string();
        instances.emplace_back(entry.path().string());
    }
  }

  std::ranges::sort(instances);
  return instances;
}


}  // namespace atlantis::benchmark

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
void defaultArguments(::benchmark::internal::Benchmark* benchmark) {
  F::populateInstances();
  for (size_t instance = 0; instance < F::size(); ++instance) {
    // TODO: re-add threads
    // for (Int numThreads = 1; numThreads <= 16; numThreads *= 2) {
    Int numThreads = 1;
    for (Int searchType = 0; searchType <= 2; ++searchType) {
      benchmark->Args({static_cast<long>(instance), numThreads, searchType});
      if (numThreads == 1) {
        break;
      }
    }
#ifndef NDEBUG
    if (numThreads >= 2) {
      return;
    }
#endif
    // }
  }
}

inline std::vector<std::chrono::milliseconds> defaultTimelimits() {
#ifndef NDEBUG
  return {std::chrono::milliseconds(1000), std::chrono::milliseconds(2000)};
#else
  // TODO: Add 180s timeout back
  return {std::chrono::milliseconds(5000), std::chrono::milliseconds(30000),
          std::chrono::milliseconds(60000)};
#endif
}

inline std::vector<std::string> createInstances(const std::string& relDir) {
  std::vector<std::string> instances;
  if (!std::filesystem::exists(relDir)) {
    return {};
  }

  // Run only a small subset of instances
  // FIXME: this crashes at the end when running only nQueens
  const std::vector<std::string> fileSet = {
      // nQueens
      "16.fzn",
      "20.fzn",
      "24.fzn",
      // TSP
      "n20w120.001.fzn",
      "n60w140.001.fzn",
      "n100w140.001.fzn",
      // Knapsack
      // NOTE: Knapsack is not currently included in the filter. Doing so seems
      // to make the benchmarks crash for some reason.
      // "f10_l-d_kp_20_879.fzn",
      // "knapPI_1_500_1000_1.fzn", "knapPI_3_10000_1000_1.fzn"
  };
  for (const auto& entry : std::filesystem::directory_iterator(relDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".fzn") {
      if (std::string file = entry.path().filename().string();
          std::ranges::find(fileSet, file) != fileSet.end()) {
        const std::string dirPath = entry.path().parent_path().string();
        instances.emplace_back(entry.path().string());
      }
    }
  }

  std::ranges::sort(instances);
  return instances;
}

}  // namespace atlantis::benchmark

#pragma once

#include <functional>

#include "atlantis/types.hpp"

namespace atlantis::benchmark {

inline search::SearchType intToSearchType(Int state) {
  return static_cast<search::SearchType>(state);
}

template <class F>
void defaultArguments(::benchmark::internal::Benchmark* benchmark) {
  F::populateInstances();
  for (size_t instance = 0; instance < F::size(); ++instance) {
    for (Int numThreads = 1; numThreads <= 8; numThreads *= 2) {
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
    }
  }
}

inline std::vector<std::chrono::milliseconds> defaultTimelimits() {
#ifndef NDEBUG
  return {std::chrono::milliseconds(1000), std::chrono::milliseconds(2000)};
#else
  std::vector<std::chrono::milliseconds> result;
  for (constexpr int times_ms[] = {1, 2, 5, 10, 15, 20, 25, 30, 45, 60, 90, 120, 150, 180, 210, 240, 270, 300};
       const int ms : times_ms) {
    result.emplace_back(ms * 1000);
  }
  return result;
#endif
}

inline std::vector<std::string> createInstances(const std::string& relDir) {
  std::vector<std::string> instances;
  if (!std::filesystem::exists(relDir)) {
    return {};
  }

  // Run only a small subset of instances
  const std::vector<std::string> fileSet = {
      // Knapsack
      "f10_l-d_kp_20_879.fzn",
      "knapPI_1_100_1000_1.fzn",
      "knapPI_1_500_1000_1.fzn",
      "knapPI_1_1000_1000_1.fzn",
      "knapPI_1_5000_1000_1.fzn",
      "knapPI_1_10000_1000_1.fzn",
      // n-Queens
      "16.fzn",
      "48.fzn",
      "64.fzn",
      "128.fzn",
      "256.fzn",
      "512.fzn",
      "768.fzn",
      "1024.fzn",
      "2048.fzn",
      // TSP / TSPTW
      "n100w140.001.fzn",
      "n100w140.002.fzn",
      "n100w140.003.fzn",
      "n100w140.004.fzn",
      "n100w140.005.fzn",
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

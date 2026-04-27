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
  for (constexpr int times_ms[] = {1, 2, 5, 10, 15, 20, 25, 30, 45, 60, 90, 120,
                                   150, 180, 210, 240, 270, 300};
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
      "/knapsack/f10_l-d_kp_20_879.fzn",
      "/knapsack/knapPI_1_100_1000_1.fzn",
      "/knapsack/knapPI_1_500_1000_1.fzn",
      "/knapsack/knapPI_1_1000_1000_1.fzn",
      "/knapsack/knapPI_1_5000_1000_1.fzn",
      "/knapsack/knapPI_1_10000_1000_1.fzn",
      // n-Queens
      "/n_queens/16.fzn",
      "/n_queens/48.fzn",
      "/n_queens/64.fzn",
      "/n_queens/128.fzn",
      "/n_queens/256.fzn",
      "/n_queens/512.fzn",
      "/n_queens/768.fzn",
      "/n_queens/1024.fzn",
      "/n_queens/2048.fzn",
      // TSP
      "/tsp/n100w140.001.fzn",
      "/tsp/n100w140.002.fzn",
      "/tsp/n100w140.003.fzn",
      "/tsp/n100w140.004.fzn",
      "/tsp/n100w140.005.fzn",
      // tsptw
      "/tsptw/n20w140.001.fzn",
      "/tsptw/n40w140.001.fzn",
      "/tsptw/n60w140.001.fzn",
      "/tsptw/n80w140.001.fzn",
      "/tsptw/n100w140.001.fzn",
};
  for (const auto& entry : std::filesystem::directory_iterator(relDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".fzn") {
      for (const auto& model : fileSet) {
        if (std::string file = entry.path().string();
            file.ends_with(model)) {
          printf("Adding instance %s\n", file.c_str());
          instances.emplace_back(file);
          break;
            }
      }
    }
  }

  std::ranges::sort(instances);
  return instances;
}

}  // namespace atlantis::benchmark

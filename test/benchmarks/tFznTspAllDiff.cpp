#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <iostream>

#include "solver.hpp"

TEST(FznTspAllDiff, Solve) {
  std::filesystem::path modelFilePath("../fzn-models/tsp_alldiff.fzn");
  Solver solver(modelFilePath, search::AnnealingScheduleFactory(), std::time(nullptr), std::chrono::milliseconds(1000));
  auto statistics = solver.solve();
  // Don't log to std::cout, since that would interfere with MiniZinc.
  statistics.display(std::cerr);
}

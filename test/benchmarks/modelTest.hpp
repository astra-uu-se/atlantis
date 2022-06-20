#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <iostream>

#include "solver.hpp"

static void testModelFile(const char* modelFile) {
  std::filesystem::path modelFilePath(modelFile);
  Solver solver(modelFilePath, search::AnnealingScheduleFactory(),
                std::time(nullptr), std::chrono::milliseconds(1000));
  logging::Logger logger(stdout, logging::Level::ERR);
  auto statistics = solver.solve(logger);
  // Don't log to std::cout, since that would interfere with MiniZinc.
  statistics.display(std::cerr);
}

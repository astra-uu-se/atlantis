#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "fznBackend.hpp"

namespace atlantis::testing {

static void testModelFile(const char* modelFile) {
  std::filesystem::path modelFilePath(
      (std::string(FZN_DIR) + "/" + modelFile).c_str());
  FznBackend backend(modelFilePath, search::AnnealingScheduleFactory(),
                     std::time(nullptr), std::chrono::milliseconds(1000));
  logging::Logger logger(stdout, logging::Level::ERR);
  auto statistics = backend.solve(logger);
  // Don't log to std::cout, since that would interfere with MiniZinc.
  statistics.display(std::cerr);
}

}  // namespace atlantis::testing
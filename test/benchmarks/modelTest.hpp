#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "atlantis/fznBackend.hpp"

namespace atlantis::testing {

static void testModelFile(const char* modelFile,
                          logging::Level logLvl = logging::Level::LVL_ERROR,
                          std::optional<std::uint_fast32_t> seed = {}) {
  std::filesystem::path modelFilePath(
      (std::string(FZN_DIR) + "/" + modelFile).c_str());
  logging::Logger logger(stdout, logLvl);
  FznBackend backend(logger, std::move(modelFilePath));
  if (seed.has_value()) {
    backend.setRandomSeed(seed.value());
  }
  backend.setTimelimit(std::chrono::seconds(2));
  auto statistics = backend.solve(logger);
  // Don't log to std::cout, since that would interfere with MiniZinc.
  statistics.display(std::cerr);
}

}  // namespace atlantis::testing

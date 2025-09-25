#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "atlantis/fznBackend.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/searchStatistics.hpp"

namespace atlantis::testing {

static void testModelFile(const char* modelFile,
                          logging::Level logLvl = logging::Level::LVL_ERROR,
                          std::optional<std::uint_fast32_t> seed = {}) {
  std::filesystem::path modelFilePath(
      (std::string(FZN_DIR) + "/" + modelFile).c_str());
  logging::Logger logger(stdout, logLvl);
  FznBackend backend(logger, std::move(modelFilePath), 4,
                     search::SearchType::BEAMSEARCH);
  if (seed.has_value()) {
    backend.setRandomSeed(seed.value());
  }
  backend.setTimelimit(std::chrono::seconds(2));
  backend.solve(logger);
}

}  // namespace atlantis::testing

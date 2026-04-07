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

static void testModelFile(
    const char* modelFile,
    std::unordered_set<Int> validObjectives = std::unordered_set<Int>{},
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
  std::optional<search::SavedAssignment> solution{};
  backend.setOnSolution(
      [&solution, &validObjectives](const search::SavedAssignment& sol) {
        solution = sol;
        EXPECT_EQ(sol.getCost().getViolation(), 0);
        if (!validObjectives.empty()) {
          EXPECT_TRUE(validObjectives.contains(sol.getCost().getObjective()))
              << "Objective: " << sol.getCost().getObjective();
        }
      });
  backend.setOnFinish([&](const FznBackend::SolveOutcome outcome) {
    EXPECT_EQ(outcome == FznBackend::SolveOutcome::SATISFIABLE,
              solution.has_value());
  });
  backend.solve(logger);
  backend.join(logger);
}

}  // namespace atlantis::testing

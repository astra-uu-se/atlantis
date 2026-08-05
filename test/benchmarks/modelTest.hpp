#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "../../build-release/_deps/googletest-src/googlemock/include/gmock/gmock-matchers.h"
#include "atlantis/fznBackend.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/searchStatistics.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::testing {

using ::testing::ContainerEq;

inline FznBackend createBackend(const char* modelFile, logging::Logger& logger,
                                const std::optional<std::uint_fast32_t> seed) {
  std::filesystem::path modelFilePath(
      (std::string(FZN_DIR) + "/" + modelFile).c_str());

  FznBackend backend(logger, std::move(modelFilePath), 1,
                     search::SearchType::PARALLEL);
  if (seed.has_value()) {
    backend.setRandomSeed(seed.value());
  }
  backend.setTimelimit(std::chrono::seconds(5));
  return backend;
}

static void testModelFile(
    const char* modelFile,
    const std::function<void(const search::SavedAssignment&,
                             const std::optional<std::vector<
                                 std::shared_ptr<search::SearchStatistics>>>&)>&
        onSolution,
    const std::function<void(FznBackend::SolveOutcome)>& onFinish,
    const logging::Level logLvl = logging::Level::LVL_ERROR,
    const std::optional<std::uint_fast32_t> seed = {}) {
  logging::Logger logger(stdout, logLvl);
  auto backend = createBackend(modelFile, logger, seed);
  backend.setOnSolution(onSolution);
  backend.setOnFinish(onFinish);
  backend.solve(logger);
  backend.join(logger);
}

static void testModelFile(
    const char* modelFile, const std::vector<std::vector<Int>>& expectedOutputs,
    const logging::Level logLvl = logging::Level::LVL_ERROR,
    const std::optional<std::uint_fast32_t> seed = {}) {
  logging::Logger logger(stdout, logLvl);
  auto backend = createBackend(modelFile, logger, seed);
  std::optional<search::SavedAssignment> solution{};
  backend.setOnSolution(
      [&expectedOutputs, &solution](
          const search::SavedAssignment& sol,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
        solution = sol;
        EXPECT_EQ(sol.cost().violation(), 0);
        const std::vector<Int>& actualOutput = sol.getOutputValues();
        size_t outputIndex = expectedOutputs.size();
        for (size_t i = 0; i < expectedOutputs.size(); ++i) {
          if (outputIndex < expectedOutputs.size()) {
            break;
          }
          EXPECT_EQ(expectedOutputs.at(i).size(), actualOutput.size());
          bool isValid = expectedOutputs.at(i).size() == actualOutput.size();
          if (!isValid) {
            break;
          }
          for (size_t j = 0; j < expectedOutputs.at(i).size(); ++j) {
            if (actualOutput.at(j) != expectedOutputs.at(i).at(j)) {
              isValid = false;
              break;
            }
          }
          if (isValid) {
            outputIndex = i;
            break;
          }
        }
        EXPECT_LT(outputIndex, expectedOutputs.size());
        EXPECT_THAT(actualOutput, ContainerEq(expectedOutputs.at(outputIndex)));
      });
  backend.setOnFinish([&](const FznBackend::SolveOutcome outcome) {
    EXPECT_EQ(outcome == FznBackend::SolveOutcome::SATISFIABLE,
              solution.has_value());
  });
  backend.solve(logger);
  backend.join(logger);
}

static void testModelFile(
    const char* modelFile,
    const std::unordered_set<Int>& validObjectives = std::unordered_set<Int>{},
    const logging::Level logLvl = logging::Level::LVL_ERROR,
    const std::optional<std::uint_fast32_t> seed = {}) {
  logging::Logger logger(stdout, logLvl);
  auto backend = createBackend(modelFile, logger, seed);
  std::optional<search::SavedAssignment> solution{};
  backend.setOnSolution(
      [&backend, &solution, &validObjectives](
          const search::SavedAssignment& sol,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
        solution = sol;
        EXPECT_EQ(sol.cost().violation(), 0);
        const Int objective =
            backend.problemType() == fznparser::ProblemType::MAXIMIZE
                ? overflow::saturatingAbs(sol.cost().objective())
                : sol.cost().objective();
        if (!validObjectives.empty()) {
          EXPECT_TRUE(validObjectives.contains(objective))
              << "Objective: " << objective;
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

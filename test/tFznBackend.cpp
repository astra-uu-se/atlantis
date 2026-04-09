#include <gtest/gtest.h>

#include <optional>

#include "atlantis/fznBackend.hpp"
#include "atlantis/logging/logger.hpp"

namespace atlantis::testing {

TEST(FznBackendTest, BuildInconsistencyReportsUnsatisfiable) {
  fznparser::Model model;
  auto a = std::make_shared<fznparser::IntVar>(0, "a");
  auto b = std::make_shared<fznparser::IntVar>(1, "b");
  model.addVar(a);
  model.addVar(b);
  model.addConstraint(fznparser::Constraint{
      "int_eq",
      std::vector<fznparser::Arg>{fznparser::IntArg(a), fznparser::IntArg(b)}});

  logging::Logger logger(stderr, logging::Level::LVL_ERROR);
  FznBackend backend(std::move(model), 1, search::SearchType::BEAMSEARCH);

  std::optional<FznBackend::SolveOutcome> outcome;
  bool sawSolution = false;
  backend.setOnSolution(
      [&](const search::SavedAssignment&,
          const std::optional<
              std::vector<std::shared_ptr<search::SearchStatistics>>>&) {
        sawSolution = true;
      });
  backend.setOnFinish([&](const FznBackend::SolveOutcome actualOutcome) {
    outcome = actualOutcome;
  });

  EXPECT_NO_THROW(backend.solve(logger));
  EXPECT_NO_THROW(backend.join(logger));
  ASSERT_TRUE(outcome.has_value());
  EXPECT_EQ(outcome.value(), FznBackend::SolveOutcome::UNSATISFIABLE);
  EXPECT_FALSE(sawSolution);
}

}  // namespace atlantis::testing
